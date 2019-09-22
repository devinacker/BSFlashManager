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
	virtual QByteArray readBytes(quint8 bank, quint16 addr, unsigned size, bool *ok = nullptr) = 0;
	virtual bool writeByte(quint8 bank, quint16 addr, quint8 data) = 0;

signals:
	void usbLogMessage(const QString&);

protected:

	class USBException : public std::exception 
	{
	public:
		USBException(const QString& msg) 
			: std::exception()
			, msg(msg.toUtf8())
		{}

		const char *what() const { return msg.constData(); }

	private:
		QByteArray msg;
	};

	void writeControlPacket(quint8 bRequest, quint16 wValue, quint16 wIndex, quint16 wLength = 1);
	QByteArray inData;

private:

	struct
	{
		quint16 vid, pid;
	} usbDevice;
	struct libusb_context *usbContext;
	struct libusb_device_handle *usbHandle;
};
