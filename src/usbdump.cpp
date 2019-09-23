
#include "usbdump.h"
#include "usb/inlretro.h"

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qdebug.h>

// ----------------------------------------------------------------------------
USBDumpDialog::USBDumpDialog(USBDevice::DeviceType deviceType, QWidget *parent)
	: QDialog(parent)
	, deviceType(deviceType)
	, dumpThread(nullptr)
{
	ui.setupUi(this);

	ui.buttonCancel->hide();

	connect(ui.buttonBrowse, SIGNAL(clicked(bool)), this, SLOT(browse()));
	connect(ui.buttonStartDump, SIGNAL(clicked(bool)), this, SLOT(startDump()));
	connect(ui.buttonCancel, SIGNAL(clicked(bool)), this, SLOT(cancelDump()));
	connect(ui.buttonClose, SIGNAL(clicked(bool)), this, SLOT(reject()));

	showMessage(tr("Press Start to begin dumping."));
	showMessage(tr("If the file is dumped successfully, it will be automatically opened in the main window."));
}

// ----------------------------------------------------------------------------
QString USBDumpDialog::dump()
{
	if (this->browse() && this->exec())
	{
		return ui.editOutputPath->text();
	}

	return QString();
}

// ----------------------------------------------------------------------------
void USBDumpDialog::reject()
{
	cancelDump();
	if (dumpThread != nullptr)
	{
		dumpThread->wait();
	}
	QDialog::reject();
}

// ----------------------------------------------------------------------------
bool USBDumpDialog::browse()
{
	QString path = QFileDialog::getSaveFileName(this,
		this->windowTitle(), ui.editOutputPath->text(), "(*.bs)");

	if (!path.isEmpty())
	{
		ui.editOutputPath->setText(path);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
void USBDumpDialog::startDump()
{
	QString outPath = ui.editOutputPath->text();
	if (outPath.isEmpty())
	{
		QMessageBox::warning(this, this->windowTitle(),
			tr("Output file name cannot be empty."));
		return;
	}

	// UI setup to start dumping
	ui.editOutputPath->setEnabled(false);
	ui.buttonBrowse->setEnabled(false);
	ui.buttonStartDump->hide();
	ui.buttonCancel->show();
	ui.progressBar->setRange(0, 0);

	dumpThread = new USBDumpThread(deviceType, outPath, this);

	connect(dumpThread, SIGNAL(showMessage(QString)), this, SLOT(showMessage(QString)));
	connect(dumpThread, SIGNAL(dumpProgress(int, int)), this, SLOT(setProgress(int, int)));
	connect(dumpThread, SIGNAL(dumpFinished()), this, SLOT(accept()));

	dumpThread->start();
	while (!dumpThread->isFinished())
	{
		qApp->processEvents();
	}

	delete dumpThread;
	dumpThread = nullptr;

	// restore UI
	ui.editOutputPath->setEnabled(true);
	ui.buttonBrowse->setEnabled(true);
	ui.buttonStartDump->show();
	ui.buttonCancel->hide();
	ui.progressBar->setRange(0, 1);
	ui.progressBar->setTextVisible(false);
}

// ----------------------------------------------------------------------------
void USBDumpDialog::cancelDump()
{
	if (dumpThread != nullptr)
	{
		dumpThread->requestInterruption();
	}
}

// ----------------------------------------------------------------------------
void USBDumpDialog::setProgress(int val, int max)
{
	ui.progressBar->setRange(0, max);
	ui.progressBar->setValue(val);
}

// ----------------------------------------------------------------------------
void USBDumpDialog::showMessage(const QString& msg)
{
	qDebug() << msg;
	ui.editDumpLog->appendPlainText(msg);
}

// ----------------------------------------------------------------------------
USBDumpThread::USBDumpThread(USBDevice::DeviceType deviceType, const QString &outPath, QObject *parent)
	: QThread(parent)
	, outPath(outPath)
{
	switch (deviceType)
	{
	case USBDevice::INLRetro:
		this->usbDevice = new INLRetroDevice(this);
		break;

	default:
		this->usbDevice = nullptr;
	}

	if (this->usbDevice)
	{
		connect(this->usbDevice, SIGNAL(usbLogMessage(QString)), this, SIGNAL(showMessage(QString)));
	}
}

// ----------------------------------------------------------------------------
void USBDumpThread::run()
{
	QFile file(outPath);

	bool ok = true;

	if (this->usbDevice->open() && file.open(QFile::WriteOnly))
	{
		emit showMessage(tr("USB device opened successfully."));

		// first try to detect memory pack
		// restore default page buffer settings
		this->usbDevice->writeByte(0xc0, 0x0000, 0x38);
		this->usbDevice->writeByte(0xc0, 0x0000, 0xd0);

		// TODO... fix USB write commands to the programmer
#if 0
		// swap in the vendor info page in the flash chip and see what we find
		this->usbDevice->writeByte(0xc0, 0x0000, 0x72);
		this->usbDevice->writeByte(0xc0, 0x0000, 0x75);
		QByteArray flashInfo = this->usbDevice->readBytes(0xc0, 0xff00, 16);
		this->usbDevice->writeByte(0xc0, 0x0000, 0xff);
		if (flashInfo.size() < 16
			|| flashInfo[0] != 'M'
			|| flashInfo[2] != 'P'
			|| flashInfo[4] > (char)0x80)
		{
			emit showMessage(tr("No valid memory pack detected."));
			ok = false;
		}

		quint8 flashType = (uchar)flashInfo[6] >> 4;
		quint8 flashSize = 2 << (((uchar)flashInfo[6]) - 8);
		if (flashSize > 32)
		{
			qDebug() << "warning: bogus flash size" << flashSize << "blocks";
			flashSize = 8;
		}
		emit showMessage(tr("Type %1 memory pack detected, %2 blocks.").arg(flashType).arg(flashSize));
#else
		quint8 flashSize = 8;
#endif

		for (unsigned i = 0; ok && i < flashSize << 1; i++)
		{
			if (!(i & 1))
			{
				emit showMessage(tr("Dumping block %1 of %2...").arg((i >> 1) + 1).arg(flashSize));
			}

			emit dumpProgress(i, flashSize << 1);
			file.write(this->usbDevice->readBytes(0xc0 + i, 0x0000, 1 << 16, &ok));

			yieldCurrentThread();
			if (this->isInterruptionRequested())
			{
				ok = false;
			}
		}
		if (this->isInterruptionRequested())
		{
			emit showMessage(tr("Dump cancelled."));
		}
		else if (ok)
		{
			emit showMessage(tr("Full file dumped successfully."));

			emit dumpFinished();
		}
		else
		{
			emit showMessage(tr("File dump failed."));
		}
	}
	else
	{
		emit showMessage(tr("USB device open failed."));
		ok = false;
	}

	this->usbDevice->close();

}
