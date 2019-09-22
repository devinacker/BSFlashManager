#pragma once

#include <QtWidgets/QDialog>
#include "ui_usbdump.h"

class USBDevice;

class USBDumpDialog : public QDialog
{
	Q_OBJECT

public:

	enum DeviceType
	{
		INLRetro,
	};

	USBDumpDialog(DeviceType deviceType, QWidget *parent = Q_NULLPTR);
	~USBDumpDialog() {}
	
	QString dump();

private slots:
	bool browse();

	void startDump();
	void cancelDump();

	void showMessage(const QString&);

private:
	bool cancelled;

	USBDevice *usbDevice;
	Ui::USBDumpDialog ui;
};
