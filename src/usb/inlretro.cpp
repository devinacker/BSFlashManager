#include "inlretro.h"

#include <QThread>

enum INLRequest {
	requestIO         = 0x02,
	requestSNES       = 0x04,
	requestBuffer     = 0x05,
	requestOperation  = 0x07,
	requestBootloader = 0x0a,
};

// IO opcodes
#define IO_RESET  0x0000
#define SNES_INIT 0x0002

// SNES cart opcodes
#define SNES_SET_BANK   0x0000
#define SNES_ROM_RD     0x0001
#define SNES_ROM_WR(b) ((b<<8)|0x0002)
#define SNES_SYS_RD     0x0005
#define SNES_SYS_WR(b) ((b<<8)|0x0006)

// buffer opcodes
#define RAW_BUFFER_RESET        0x0000
#define SET_MEM_N_PART(b)       ((b<<8)|0x0030)
	#define SNESROM_PAGE 0x24
	#define MASKROM 0xdd
#define SET_MAP_N_MAPVAR(b)     ((b<<8)|0x0032)
#define GET_CUR_BUFF_STATUS     0x0061
	#define STATUS_DUMPED 0xd8
#define BUFF_PAYLOAD            0x0070
#define ALLOCATE_BUFFER(n,b)    ((b<<8)|0x0080|n)
#define SET_RELOAD_PAGENUM(n,b) ((b<<8)|0x0090|n)

// operations
#define SET_OPERATION 0x0000
	#define OPERATION_RESET     0x0001
	#define OPERATION_STARTDUMP 0x00d2

// bootloader opcodes
#define GET_APP_VER 0x000c // len = 3

// ----------------------------------------------------------------------------
INLRetroDevice::INLRetroDevice(QObject *parent)
	: USBDevice(0x16c0, 0x05dc, parent)
{
	currentBank = 0;
}

// ----------------------------------------------------------------------------
bool INLRetroDevice::open()
{
	if (USBDevice::open())
	{
		try
		{
			// validate vendor & product names
			QString vendor, product;
			getVendorAndProductName(vendor, product);
			if (vendor != "InfiniteNesLives.com" || product != "INL Retro-Prog")
			{
				throw USBException(tr("Matching USB device not found."));
			}

			writeControlPacket(requestBootloader, GET_APP_VER, 0, 3);
			if (inData[1] < '\x01' && inData[2] < '\x03')
			{
				emit usbLogMessage(tr("WARNING: Unsupported INL-Retro firmware version. Dumping may not succeed.\n"
					"Make sure your programmer firmware is up to date. See https://gitlab.com/InfiniteNesLives/INL-retro-progdump for info."));
			}

			writeControlPacket(requestIO, IO_RESET, 0);
			writeControlPacket(requestIO, SNES_INIT, 0);

			currentBank = 0;

			return true;
		}
		catch (USBException &e) 
		{
			emit usbLogMessage(e.what());
		}
	}

	close();
	return false;
}

// ----------------------------------------------------------------------------
quint8 INLRetroDevice::readByte(quint8 bank, quint16 addr, bool *ok)
{
	bool bOk = false;
	quint8 val = 0;

	quint16 value = SNES_SYS_RD;
	if ((bank & 0x40) || (addr & 0x8000))
	{
		value = SNES_ROM_RD; // assert ROMSEL
	}

	try
	{
		setBank(bank); 
		writeControlPacket(requestSNES, value, addr, 3);

		if (this->inData[1] >= '\x01')
		{
			val = this->inData[2];
			bOk = true;
		}
	}
	catch (USBException &e)
	{
		emit usbLogMessage(e.what());
	}

	if (ok) *ok = bOk;
	return val;
}

// ----------------------------------------------------------------------------
QByteArray INLRetroDevice::readBytes(quint8 bank, quint16 addr, unsigned size, bool *ok)
{
	bool bOk = false;
	QByteArray readData;

	try
	{
		setBank(bank);

		// reset buffers
		writeControlPacket(requestOperation, SET_OPERATION, OPERATION_RESET);
		writeControlPacket(requestBuffer,    RAW_BUFFER_RESET, 0);

		// initialize buffers for 128 byte reads
		// buffer 0: id 0x00, bank offset 0x00, reload 1
		// buffer 1: id 0x80, bank offset 0x04, reload 1
		writeControlPacket(requestBuffer, ALLOCATE_BUFFER(0, 4), 0x0000);
		writeControlPacket(requestBuffer, ALLOCATE_BUFFER(1, 4), 0x8004);
		writeControlPacket(requestBuffer, SET_RELOAD_PAGENUM(0, 1), 0x0000);
		writeControlPacket(requestBuffer, SET_RELOAD_PAGENUM(1, 1), 0x0000);

		// set up buffer read pointers
		// mapper number = CPU addr page number, mapper variation = always 0
		// TODO: handle start addresses that aren't page-aligned
		writeControlPacket(requestBuffer, SET_MEM_N_PART(0), (SNESROM_PAGE << 8) | MASKROM);
		writeControlPacket(requestBuffer, SET_MEM_N_PART(1), (SNESROM_PAGE << 8) | MASKROM);
		writeControlPacket(requestBuffer, SET_MAP_N_MAPVAR(0), addr & 0xff00);
		writeControlPacket(requestBuffer, SET_MAP_N_MAPVAR(1), addr & 0xff00);

		// start dump
		writeControlPacket(requestOperation, SET_OPERATION, OPERATION_STARTDUMP);

		while (readData.size() < size)
		{
			// wait for read buffer
			unsigned waitCount = 0;
			while (true)
			{
				writeControlPacket(requestBuffer, GET_CUR_BUFF_STATUS, 0, 3);
				if (this->inData.size() < 3 || this->inData[2] != (char)STATUS_DUMPED)
				{
					QThread::msleep(10);
					if (++waitCount > 32)
					{
						throw USBException(tr("Timed out waiting for INL Retro to fill read buffer"));
					}
				}
				else
				{
					break;
				}
			}

			// get data (no return value, only data)
			USBDevice::writeControlPacket(requestBuffer, BUFF_PAYLOAD, 0, 128);
			readData += this->inData;
		}

		// we're finished; get out of dump mode again
		writeControlPacket(requestOperation, SET_OPERATION, OPERATION_RESET);
		writeControlPacket(requestBuffer, RAW_BUFFER_RESET, 0);

		bOk = true;
	}
	catch (USBException &e)
	{
		emit usbLogMessage(e.what());
	}

	if (ok) *ok = bOk;
	return readData;
}

// ----------------------------------------------------------------------------
bool INLRetroDevice::writeByte(quint8 bank, quint16 addr, quint8 data)
{
	quint16 value = SNES_SYS_WR(data);
	if ((bank & 0x40) || (addr & 0x8000))
	{
		value = SNES_ROM_WR(data); // assert ROMSEL
	}

	try
	{
		setBank(bank);
		writeControlPacket(requestSNES, value, addr);
		return true;
	}
	catch (USBException &e)
	{
		emit usbLogMessage(e.what());
	}

	return false;
}

// ----------------------------------------------------------------------------
void INLRetroDevice::setBank(quint8 bank)
{
	if (bank != currentBank)
	{
		writeControlPacket(requestSNES, SNES_SET_BANK, bank);
		currentBank = bank;
	}
}

// ----------------------------------------------------------------------------
void INLRetroDevice::writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength)
{
	USBDevice::writeControlPacket(bRequest, wValue, wIndex, wLength);
	if (inData[0] != '\0')
	{
		throw USBException(tr("Control request error: bRequest=0x%1, wValue=0x%2, wIndex=0x%3, rc=0x%4")
			.arg(bRequest, 2, 16, QChar('0'))
			.arg(wValue, 4, 16, QChar('0'))
			.arg(wIndex, 4, 16, QChar('0'))
			.arg((quint8)inData[0], 2, 16, QChar('0')));
	}
}
