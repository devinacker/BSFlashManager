#pragma once

#include <QtWidgets/QDialog>
#include <qthread.h>
#include "ui_usbdump.h"
#include "usb/device.h"

class USBDumpThread;

class USBDumpDialog : public QDialog
{
	Q_OBJECT

public:

	USBDumpDialog(USBDevice::DeviceType deviceType, QWidget *parent = Q_NULLPTR);
	~USBDumpDialog() {}
	
	QString dump();

public slots:
	void reject();

private slots:
	bool browse();

	void startDump();
	void cancelDump();

	void setProgress(int val, int max);
	void showMessage(const QString&);

private:
	USBDevice::DeviceType deviceType;
	USBDumpThread *dumpThread;
	Ui::USBDumpDialog ui;
};

class USBDumpThread : public QThread
{
	Q_OBJECT

public:
	USBDumpThread(USBDevice::DeviceType deviceType, const QString &outPath, QObject *parent = Q_NULLPTR);

signals:
	void showMessage(const QString&);

	void dumpProgress(int val, int max);
	void dumpFinished();

protected:
	void run();

private:
	QString outPath;
	USBDevice *usbDevice;
};
