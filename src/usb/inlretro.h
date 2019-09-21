#pragma once

#include "device.h"

class INLRetroDevice : public USBDevice
{
	Q_OBJECT

public:
	INLRetroDevice(QObject *parent = nullptr);
	~INLRetroDevice() {}

	bool open();

	quint8 readByte(quint8 bank, quint16 addr, bool *ok = nullptr);
	QByteArray readBytes(quint8 bank, quint16 addr, unsigned size, bool *ok = nullptr);
	bool writeByte(quint8 bank, quint16 addr, quint8 data);

private:
	void setBank(quint8 bank);

	quint8 currentBank;
};
