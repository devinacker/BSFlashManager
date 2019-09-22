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
		this->usbHandle = libusb_open_device_with_vid_pid(this->usbContext, 
			this->usbDevice.vid, this->usbDevice.pid);

		if (!this->usbHandle) return false;

		// TODO allow specifying these, but the defaults should be ok...
		libusb_set_configuration(this->usbHandle, 1);
		libusb_claim_interface(this->usbHandle, 0);
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
}
