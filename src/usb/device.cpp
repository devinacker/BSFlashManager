#include "device.h"

#include "libusb-1.0/libusb.h"

#define TRANSFER_TIMEOUT 250 // in ms

// ----------------------------------------------------------------------------
USBDevice::USBDevice(quint16 vid, quint16 pid, QObject *parent)
	: QObject(parent)
	, usbHandle(nullptr)
{
	this->usbDevice.vid = vid;
	this->usbDevice.pid = pid;
	this->usbDevice.in_ep  = 0;
	this->usbDevice.out_ep = 0;

	libusb_init(&this->usbContext);
#ifdef QT_DEBUG
	libusb_set_debug(this->usbContext, LIBUSB_LOG_LEVEL_DEBUG);
#endif
}

// ----------------------------------------------------------------------------
USBDevice::~USBDevice()
{
	close();
	libusb_exit(this->usbContext);
}

// ----------------------------------------------------------------------------
bool USBDevice::open()
{
	if (!this->usbHandle)
	{
		this->usbDevice.in_ep = 0;
		this->usbDevice.out_ep = 0;
		this->usbHandle = libusb_open_device_with_vid_pid(this->usbContext, 
			this->usbDevice.vid, this->usbDevice.pid);

		if (!this->usbHandle) return false;

		// TODO allow specifying these, but the defaults should be ok...
		libusb_set_configuration(this->usbHandle, 1);
		libusb_claim_interface(this->usbHandle, 0);

		// find bulk endpoints on the selected config and interface
		libusb_config_descriptor *config;
		libusb_device *device = libusb_get_device(this->usbHandle);
		if (libusb_get_active_config_descriptor(device, &config) == 0)
		{
			const libusb_interface *interface = config->interface;
			if (config->bNumInterfaces > 0 && interface->num_altsetting > 0)
			{
				const libusb_interface_descriptor *desc = interface->altsetting;

				for (int i = 0; i < desc->bNumEndpoints; i++)
				{
					const libusb_endpoint_descriptor *endpoint = desc->endpoint + i;
					
					if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
					{
						this->usbDevice.in_ep = endpoint->bEndpointAddress;
					}
					else
					{
						this->usbDevice.out_ep = endpoint->bEndpointAddress;
					}

					if (this->usbDevice.in_ep && this->usbDevice.out_ep)
					{
						break;
					}					
				}
			}

			libusb_free_config_descriptor(config);
		}
	}

	// was already open
	return true;
}

// ----------------------------------------------------------------------------
void USBDevice::close()
{
	if (this->usbHandle)
	{
		libusb_release_interface(this->usbHandle, 0);
		libusb_close(this->usbHandle);
		this->usbHandle = nullptr;
	}
}

// ----------------------------------------------------------------------------
QByteArray USBDevice::readBulk(quint8 endpoint, int length, unsigned blockSize)
{
	QByteArray data;
	data.resize(length);
	blockSize &= ~63;

	if (this->usbHandle)
	{
		int offset = 0;
		while (offset < length)
		{
			int transferred;
			int rc = libusb_bulk_transfer(this->usbHandle, LIBUSB_ENDPOINT_IN  | endpoint,
				(uchar*)data.data() + offset, qMin((int)blockSize, length - offset), &transferred, TRANSFER_TIMEOUT);

			if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT)
			{
				throw USBException(tr("Bulk read error: %1").arg(libusb_strerror((libusb_error)rc)));
			}

			offset += transferred;
		}
	}
	else
	{
		throw USBException(tr("Tried to bulk read from a closed USB device."));
	}

	return data;
}

// ----------------------------------------------------------------------------
int USBDevice::writeBulk(quint8 endpoint, const QByteArray &data, unsigned blockSize)
{
	int offset = 0;
	blockSize &= ~63;

	if (this->usbHandle)
	{
		int offset = 0;
		while (offset < data.size())
		{
			int transferred;
			int rc = libusb_bulk_transfer(this->usbHandle, LIBUSB_ENDPOINT_OUT | endpoint,
				(uchar*)data.data() + offset, qMin((int)blockSize, data.size() - offset), &transferred, TRANSFER_TIMEOUT);

			if (rc < 0 && rc != LIBUSB_ERROR_TIMEOUT)
			{
				throw USBException(tr("Bulk write error: %1").arg(libusb_strerror((libusb_error)rc)));
			}

			offset += transferred;
		}
	}
	else
	{
		throw USBException(tr("Tried to bulk write to a closed USB device."));
	}

	return offset;
}

// ----------------------------------------------------------------------------
void USBDevice::writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength)
{
	if (this->usbHandle)
	{
		this->inData.resize(wLength);

		int rc = libusb_control_transfer(this->usbHandle, 0x80 | LIBUSB_REQUEST_TYPE_VENDOR, bRequest, wValue, wIndex,
			(unsigned char*)this->inData.data(), wLength, TRANSFER_TIMEOUT);

		if (rc < 0)
		{
			throw USBException(tr("Control request error: %1").arg(libusb_strerror((libusb_error)rc)));
		}
	}
	else
	{
		throw USBException(tr("Tried to control a closed USB device."));
	}
}
