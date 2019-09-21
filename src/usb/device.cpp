#include "device.h"

#include <QtUsb/qusbdevice.h>
#include <QtUsb/qusbtransfer.h>

#define TRANSFER_TIMEOUT 100 // in ms

// ----------------------------------------------------------------------------
USBDevice::USBDevice(QObject *parent)
	: QObject(parent)
	, usbDevice(new QUsbDevice(this))
	, usbTransfer(nullptr)
{
#ifdef QT_DEBUG
	usbDevice->setLogLevel(QUsbDevice::logDebug);
#endif
}

// ----------------------------------------------------------------------------
USBDevice::~USBDevice()
{
	close();
}

// ----------------------------------------------------------------------------
bool USBDevice::open()
{
	return usbDevice->open() == QUsbDevice::statusOK;
}

// ----------------------------------------------------------------------------
void USBDevice::close()
{
	if (usbDevice->isConnected())
	{
		closeHandle();
		usbDevice->close();
	}
}

// ----------------------------------------------------------------------------
bool USBDevice::openHandle(uint type, quint8 endpointIn, quint8 endpointOut)
{
	usbTransfer = new QUsbTransfer(usbDevice, (QUsbTransfer::Type)type, endpointIn, endpointOut);

	// TODO: connect r/w signals here

	if (usbTransfer->open(QIODevice::ReadWrite))
	{
		usbTransfer->setPolling(true);
		return true;
	}

	delete usbTransfer;
	usbTransfer = nullptr;
	return false;
}

// ----------------------------------------------------------------------------
void USBDevice::closeHandle()
{
	if (usbTransfer)
	{
		usbTransfer->close();
		usbTransfer->disconnect();
		delete usbTransfer;
		usbTransfer = nullptr;
	}
}

// ----------------------------------------------------------------------------
void USBDevice::writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength)
{
	if (usbTransfer)
	{
		char packet[8];
		usbTransfer->makeControlPacket(packet, QUsbTransfer::requestVendor, (QUsbTransfer::bRequest)bRequest, wValue, wIndex, wLength);

		inData.clear();
		if (usbTransfer->write(packet, sizeof packet) == sizeof packet)
		{
			while (inData.size() < wLength)
			{
				// TODO: probably do this in a nonblocking way instead...
				if (!usbTransfer->waitForReadyRead(TRANSFER_TIMEOUT))
				{
					usbTransfer->cancelTransfer();
					throw TimeoutException();
				}

				inData += usbTransfer->readAll();
			}

		}
	}
}
