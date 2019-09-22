
#include "usbdump.h"
#include "usb/inlretro.h"

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qdebug.h>

// ----------------------------------------------------------------------------
USBDumpDialog::USBDumpDialog(DeviceType deviceType, QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	this->cancelled = false;

	switch (deviceType) 
	{
	case INLRetro:
		this->usbDevice = new INLRetroDevice(this);
		break;

	default:
		this->usbDevice = nullptr;
	}

	ui.buttonCancel->hide();

	connect(ui.buttonBrowse, SIGNAL(clicked(bool)), this, SLOT(browse()));
	connect(ui.buttonStartDump, SIGNAL(clicked(bool)), this, SLOT(startDump()));
	connect(ui.buttonCancel, SIGNAL(clicked(bool)), this, SLOT(cancelDump()));
	connect(ui.buttonClose, SIGNAL(clicked(bool)), this, SLOT(reject()));

	if (this->usbDevice)
	{
		connect(this->usbDevice, SIGNAL(usbLogMessage(QString)), this, SLOT(showMessage(QString)));
	}

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
	this->cancelled = false;

	if (!this->usbDevice)
	{
		QMessageBox::critical(this, this->windowTitle(),
			tr("No USB device was provided."));
		this->reject();
		return;
	}

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

	// TODO...
	// most of this will/should be moved into a separate thread
	QFile file(outPath);

	bool ok = true;

	if (this->usbDevice->open() && file.open(QFile::WriteOnly))
	{
		showMessage(tr("USB device opened successfully."));

		// first try to detect memory pack
		// restore default page buffer settings
		this->usbDevice->writeByte(0xc0, 0x0000, 0x38);
		this->usbDevice->writeByte(0xc0, 0x0000, 0xd0);

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
			showMessage(tr("No valid memory pack detected."));
			ok = false;
		}

		quint8 flashType = (uchar)flashInfo[6] >> 4;
		quint8 flashSize = 2 << (((uchar)flashInfo[6]) - 8);
		if (flashSize > 32)
		{
			qDebug() << "warning: bogus flash size" << flashSize << "blocks";
			flashSize = 32;
		}
		showMessage(tr("Type %1 memory pack detected, %2 blocks.").arg(flashType).arg(flashSize));
#else
		quint8 flashSize = 8;
#endif
		ui.progressBar->setRange(0, flashSize << 1);
		ui.progressBar->setTextVisible(true);

		for (unsigned i = 0; ok && i < flashSize << 1; i++)
		{
			if (!(i & 1))
			{
				showMessage(tr("Dumping block %1 of %2...").arg((i >> 1) + 1).arg(flashSize));
			}

			ui.progressBar->setValue(i);
			file.write(this->usbDevice->readBytes(0xc0 + i, 0x0000, 1 << 16, &ok));

			qApp->processEvents();
			if (this->cancelled)
			{
				ok = false;
			}
		}
		if (this->cancelled)
		{
			showMessage(tr("Dump cancelled."));
		}
		else if (ok)
		{
			showMessage(tr("Full file dumped successfully."));
		}
		else
		{
			showMessage(tr("File dump failed."));
		}
	}
	else
	{
		showMessage(tr("USB device open failed."));
		ok = false;
	}

	this->usbDevice->close();

	// restore UI
	ui.editOutputPath->setEnabled(true);
	ui.buttonBrowse->setEnabled(true);
	ui.buttonStartDump->show();
	ui.buttonCancel->hide();
	ui.progressBar->setRange(0, 1);
	ui.progressBar->setTextVisible(false);

	if (ok)
	{
		this->accept();
	}
}

// ----------------------------------------------------------------------------
void USBDumpDialog::cancelDump()
{
	this->cancelled = true;
}

// ----------------------------------------------------------------------------
void USBDumpDialog::showMessage(const QString& msg)
{
	qDebug() << msg;
	ui.editDumpLog->appendPlainText(msg);
}
