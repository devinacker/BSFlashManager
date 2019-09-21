#pragma once

#include <qobject.h>
#include <exception>

class USBDevice : public QObject
{
	Q_OBJECT

public:
	USBDevice(quint16 vid, quint16 pid, QObject *parent = nullptr);
	virtual ~USBDevice();

	virtual bool open();
	virtual void close();

	virtual quint8 readByte(quint8 bank, quint16 addr, bool *ok = nullptr) = 0;
	virtual QByteArray readBytes(quint8 bank, quint16 addr, quint16 size, bool *ok = nullptr) = 0;
	virtual bool writeByte(quint8 bank, quint16 addr, quint8 data) = 0;

protected:

	class TimeoutException : public std::exception {};

	void writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength = 1);

	struct
	{
		quint16 vid, pid;
	} usbDevice;
	struct libusb_context *usbContext;
	struct libusb_device_handle *usbHandle;
	QByteArray inData;
};
