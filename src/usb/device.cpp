#include "device.h"

#include "libusb-1.0/libusb.h"

#define TRANSFER_TIMEOUT 250 // in ms

// ----------------------------------------------------------------------------
USBDevice::USBDevice(quint16 vid, quint16 pid, QObject *parent)
	: QObject(parent)
	, usbHandle(nullptr)
{
	usbDevice.vid = vid;
	usbDevice.pid = pid;

	libusb_init(&usbContext);
#ifdef QT_DEBUG
	libusb_set_debug(usbContext, LIBUSB_LOG_LEVEL_DEBUG);
#endif
}

// ----------------------------------------------------------------------------
USBDevice::~USBDevice()
{
	close();
	libusb_exit(usbContext);
	usbContext = nullptr;
}

// ----------------------------------------------------------------------------
bool USBDevice::open()
{
	if (!usbHandle)
	{
		usbHandle = libusb_open_device_with_vid_pid(usbContext, usbDevice.vid, usbDevice.pid);

		if (!usbHandle) return false;

		// TODO allow specifying these, but the defaults should be ok...
		libusb_set_configuration(usbHandle, 1);
		libusb_claim_interface(usbHandle, 0);
	}

	// was already open
	return true;
}

// ----------------------------------------------------------------------------
void USBDevice::close()
{
	if (usbHandle)
	{
		libusb_release_interface(usbHandle, 0);
		libusb_close(usbHandle);
		usbHandle = nullptr;
	}
}

// ----------------------------------------------------------------------------
void USBDevice::writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength)
{
	if (usbHandle)
	{
		inData.resize(wLength);

		int rc = libusb_control_transfer(usbHandle, 0x80 | LIBUSB_REQUEST_TYPE_VENDOR, bRequest, wValue, wIndex,
			(unsigned char*)inData.data(), wLength, TRANSFER_TIMEOUT);

		if (rc < 0)
		{
			throw TimeoutException();
		}
	}
}
