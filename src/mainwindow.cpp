#include "mainwindow.h"
#include "mempackmodel.h"

#include "usb/inlretro.h"

#include <qtemporaryfile.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qdebug.h>
#include <QCloseEvent>

// ----------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	ui.actionNew->setShortcut(QKeySequence::New);
	ui.actionOpen->setShortcut(QKeySequence::Open);
	ui.actionSave->setShortcut(QKeySequence::Save);
	ui.actionSaveAs->setShortcut(QKeySequence::SaveAs);

	ui.actionExit->setShortcut(QKeySequence::Quit);
	
	memPackModel = new MemPackModel(this);
	ui.listView->setModel(memPackModel);

	ui.comboProgramType->addItem(tr("Normal program"),        0 << 8);
	ui.comboProgramType->addItem(tr("BS-X bytecode program"), 1 << 8);
	ui.comboProgramType->addItem(tr("SA-1 program"),          2 << 8);

	ui.comboPlaysLeft->addItem(tr("Unlimited plays"), 0x0);
	quint16 plays = 1 << 15;
	for (int i = 0; i < 16; i++)
	{
		ui.comboPlaysLeft->addItem(tr("%1 play(s) left").arg(i), plays);
		plays |= (1 << i);
	}

	connect(memPackModel, SIGNAL(modelReset()), this, SLOT(updateSelected()));
	connect(ui.listView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), 
		this, SLOT(updateSelected()));

	connect(ui.buttonAddFile, SIGNAL(clicked(bool)), this, SLOT(addFiles()));
	connect(ui.buttonDelFile, SIGNAL(clicked(bool)), this, SLOT(deleteFile()));
	connect(ui.buttonMoveFileUp, SIGNAL(clicked(bool)), this, SLOT(moveFileUp()));
	connect(ui.buttonMoveFIleDown, SIGNAL(clicked(bool)), this, SLOT(moveFileDown()));

	connect(ui.buttonApply, SIGNAL(clicked(bool)), this, SLOT(applyChanges()));
	connect(ui.buttonReset, SIGNAL(clicked(bool)), this, SLOT(updateSelected()));

	connect(ui.actionNew, SIGNAL(triggered(bool)), this, SLOT(newFile()));
	connect(ui.actionOpen, SIGNAL(triggered(bool)), this, SLOT(openFile()));
	connect(ui.actionSave, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
	connect(ui.actionSaveAs, SIGNAL(triggered(bool)), this, SLOT(saveFileAs()));
	connect(ui.actionExport, SIGNAL(triggered(bool)), this, SLOT(exportSelected()));
	connect(ui.actionExportAll, SIGNAL(triggered(bool)), this, SLOT(exportAll()));
	connect(ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(close()));

	connect(ui.actionTransferTest, SIGNAL(triggered(bool)), this, SLOT(transferTest()));

	connect(ui.actionAbout, SIGNAL(triggered(bool)), this, SLOT(about()));

	ui.infoWidget->setEnabled(false);
	newFile();
}
// ----------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
	if (promptSave())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

// ----------------------------------------------------------------------------
void MainWindow::about()
{
	QMessageBox::information(this, tr("About"),
		tr("%1 v%2\n\nCopyright 2019 Devin Acker (Revenant)")
		.arg(qApp->applicationDisplayName()).arg(qApp->applicationVersion()));
}

// ----------------------------------------------------------------------------
void MainWindow::newFile()
{
	if (!promptSave()) return;

	memPackModel->removeRows(0, memPackModel->rowCount());
	lastFileName = "";
	updateWindowTitle();
	updateBlockCount();
	setWindowModified(false);
}

// ----------------------------------------------------------------------------
void MainWindow::openFile()
{
	if (!promptSave()) return;

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
		lastFileName, tr("(*.sfc *.bs)"));

	if (!fileName.isEmpty())
	{
		MemPackItems newItems = loadAllItems(fileName);
		memPackModel->setItems(newItems);

		ui.statusBar->showMessage(tr("Opened %1.").arg(fileName));

		lastFileName = fileName;
		updateWindowTitle();
		updateBlockCount();
		setWindowModified(false);
	}
}

// ----------------------------------------------------------------------------
bool MainWindow::promptSave()
{
	if (isWindowModified())
	{
		int result = QMessageBox::question(this, tr("Save Changes"),
			tr("Save changes to %1?").arg(lastFileName), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (result == QMessageBox::Cancel)
		{
			return false;
		}
		else if (result == QMessageBox::Yes)
		{
			return saveFile();
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
bool MainWindow::saveFile()
{
	if (lastFileName.isEmpty())
	{
		return saveFileAs();
	}
	else
	{
		return saveFile(lastFileName);
	}
}

// ----------------------------------------------------------------------------
bool MainWindow::saveFile(const QString &fileName)
{
	if (fileName.isEmpty()) return false; // no file open/selected

	QTemporaryFile tempFile;
	if (tempFile.open())
	{
		// TODO: support multiple sizes of memory pack - currently assume 8 blocks
		// (saving larger ones will work, but should probably round out to a guaranteed power of two)
		uint8 *emptyFlash = new uint8[1 << 20];
		memset(emptyFlash, 0xff, 1 << 20);

		bool ok = (tempFile.write((const char*)emptyFlash, 1 << 20) == (1 << 20));
		delete[] emptyFlash;

		unsigned offset = 0;

		for (auto& item : memPackModel->items())
		{
			ok &= item.saveToFile(tempFile, offset);
			offset += item.data.size();
		}

		if (ok)
		{
			QFile::remove(fileName);
			tempFile.copy(fileName);

			lastFileName = fileName;
			updateWindowTitle();

			ui.statusBar->showMessage(tr("Saved %1.").arg(fileName));
			setWindowModified(false);
			return true;
		}
		else
		{
			QMessageBox::critical(this, tr("Save File"), tr("Unable to save %1.").arg(fileName));
		}
	}

	return false;
}

// ----------------------------------------------------------------------------
bool MainWindow::saveFileAs()
{
	const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File As"),
		lastFileName, tr("(*.bs)"));
	return saveFile(fileName);
}

// ----------------------------------------------------------------------------
void MainWindow::exportSelected()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	MemPackItems &items = memPackModel->items();

	if (row >= 0 && row < memPackModel->rowCount())
	{
		MemPackItem &item = items[row];
		ItemHeader &header = item.header;

		QString fileName = item.title.trimmed() + ".bs";
		const QString filePath = QFileDialog::getSaveFileName(this, tr("Save File As"),
			QFileInfo(lastFileName).dir().filePath(fileName), tr("(*.bs)"));

		if (!filePath.isEmpty())
		{
			QTemporaryFile tempFile;
			if (tempFile.open())
			{
				item.saveToFile(tempFile);

				QFile::remove(filePath);
				tempFile.copy(filePath);
			}
		}
	}
}

// ----------------------------------------------------------------------------
void MainWindow::exportAll()
{
	const QString outPath = QFileDialog::getExistingDirectory(this, tr("Export Files"), lastFileName);

	if (!outPath.isEmpty())
	{
		MemPackItems &items = memPackModel->items();
		QDir dir(outPath);

		for (MemPackItem& item : items)
		{
			ItemHeader &header = item.header;

			QString fileName = item.title.trimmed() + ".bs";
			const QString filePath = dir.filePath(fileName);

			if (!filePath.isEmpty())
			{
				QTemporaryFile tempFile;
				if (tempFile.open())
				{
					item.saveToFile(tempFile);

					QFile::remove(filePath);
					tempFile.copy(filePath);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
void MainWindow::transferTest()
{
	INLRetroDevice device(this);
	QFile file("./dumptest.bs");

	if (device.open() && file.open(QFile::WriteOnly))
	{
		qDebug() << "USB open succeeded";

		// first try to detect memory pack
		// restore default page buffer settings
		device.writeByte(0xc0, 0x0000, 0x38);
		device.writeByte(0xc0, 0x0000, 0xd0);

		bool ok = true;

#if 0
		// swap in the vendor info page in the flash chip and see what we find
		device.writeByte(0xc0, 0x0000, 0x72);
		device.writeByte(0xc0, 0x0000, 0x75);
		QByteArray flashInfo = device.readBytes(0xc0, 0xff00, 16);
		device.writeByte(0xc0, 0x0000, 0xff);
		if (flashInfo.size() < 16
			|| flashInfo[0] != 'M'
			|| flashInfo[2] != 'P'
			|| flashInfo[4] > (char)0x80)
		{
			qDebug() << "no valid memory pack detected";
			return;
		}

		quint8 flashType = (uchar)flashInfo[6] >> 4;
		quint8 flashSize = 2 << (((uchar)flashInfo[6]) - 8);
		if (flashSize > 32)
		{
			qDebug() << "warning: bogus flash size" << flashSize << "blocks";
			flashSize = 32;
		}
		qDebug() << "type" << flashType << "memory pack detected," << flashSize << "blocks";
#else
		quint8 flashSize = 8;
#endif
		for (unsigned i = 0; ok && i < flashSize << 1; i++)
		{
			file.write(device.readBytes(0xc0 + i, 0x0000, 1 << 16, &ok));
		}
		if (ok)
		{
			qDebug() << "full file dumped successfully";
		}
		else
		{
			qDebug() << "file dump failed";
		}
	}
	else
	{
		qDebug() << "USB open failed";
	}

	QMessageBox::information(this, tr("Transfer Test"), tr("done"));
}

// ----------------------------------------------------------------------------
void MainWindow::updateSelected()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	const MemPackItems &items = memPackModel->items();

	if (row >= 0 && row < items.size())
	{
		const MemPackItem &item = items[row];
		const ItemHeader &header = item.header;

		ui.infoWidget->setEnabled(true);

		ui.editTitle->setText(item.title);
		ui.spinBoxMonth->setValue(header.month >> 4);
		ui.spinBoxDay->setValue(header.day >> 3);
		ui.spinBoxVersion->setValue(header.version);

		// this may be either valid or a cartridge-style product ID
		int type = ui.comboProgramType->findData((uint)header.programType);
		if (type < 0) type = 0;
		ui.comboProgramType->setCurrentIndex(type);

		if (item.starts >= 0)
		{
			ui.comboPlaysLeft->setCurrentIndex(1 + item.starts);
		}
		else
		{
			ui.comboPlaysLeft->setCurrentIndex(0);
		}

		ui.checkFastROM->setChecked(header.mapMode & 0x10);
		ui.checkMuteRadio->setChecked(header.execFlags & 0x10);
		ui.checkSkipIntro->setChecked(header.execFlags & 0x80);
		ui.checkDeleted->setChecked(item.deleted);

		if (item.blocks <= 4)
		{
			ui.checkRunPSRAM->setChecked(true);
			ui.labelWarning->setText("");
		}
		else
		{
			ui.checkRunPSRAM->setChecked(false);
			ui.labelWarning->setText(tr("<b>Warning:</b> Only the first file on the Memory Pack can be larger than 4 blocks."));
		}
	}
	else
	{
		ui.infoWidget->setEnabled(false);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::applyChanges()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	MemPackItems &items = memPackModel->items();

	if (row >= 0 && row < memPackModel->rowCount())
	{
		MemPackItem &item = items[row];
		ItemHeader &header = item.header;

		item.title = ui.editTitle->text();

		header.month = ui.spinBoxMonth->value() << 4;
		header.day = ui.spinBoxDay->value() << 3;
		header.version = ui.spinBoxVersion->value();

		header.programType = ui.comboProgramType->currentData().toUInt();
		header.starts = ui.comboPlaysLeft->currentData().toUInt();
		item.starts = ui.comboPlaysLeft->currentIndex() - 1;

		if (ui.checkFastROM->isChecked())
		{
			header.mapMode |= 0x10;
		}
		else
		{
			header.mapMode &= ~0x10;
		}

		header.execFlags = (ui.checkMuteRadio->isChecked() << 4)
		                 | (ui.checkRunPSRAM->isChecked()  << 5)
		                 | (ui.checkSkipIntro->isChecked() << 7);

		item.deleted = ui.checkDeleted->isChecked();

		memPackModel->setItemHeader(row, header);
		setWindowModified(true);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::addFiles()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Add File"),
		lastFileName, tr("(*.sfc *.bs)"));

	if (!fileName.isEmpty())
	{
		MemPackItems &items = memPackModel->items();
		items += loadAllItems(fileName);

		memPackModel->setItems(items);

		ui.statusBar->showMessage(tr("Opened %1.").arg(fileName));

		updateBlockCount();
		setWindowModified(true);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::deleteFile()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	if (memPackModel->removeRow(row))
	{
		updateBlockCount();
		setWindowModified(true);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::moveFileUp()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	if (memPackModel->moveRowUp(row))
	{
		setWindowModified(true);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::moveFileDown()
{
	int row = ui.listView->selectionModel()->currentIndex().row();
	if (memPackModel->moveRowDown(row))
	{
		setWindowModified(true);
	}
}

// ----------------------------------------------------------------------------
void MainWindow::updateWindowTitle()
{
	setWindowTitle(QFileInfo(lastFileName).fileName() + "[*]");
}

// ----------------------------------------------------------------------------
void MainWindow::updateBlockCount()
{
	unsigned blocks = 0;
	for (const auto& item : memPackModel->items())
	{
		blocks += item.blocks;
	}

	ui.labelBlockUsage->setText(tr("%1 blocks used").arg(blocks));
}

// ----------------------------------------------------------------------------
MemPackItems MainWindow::loadAllItems(const QString& path)
{
	MemPackItems newItems;

	if (!path.isEmpty())
	{
		QFile file(path);

		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(this, tr("Open File"),
				tr("Couldn't open %1.").arg(path));
			return newItems;
		}

		quint32 blocks = 0;
		unsigned totalBlocks = file.size() >> 17;
		bool recoveryWarning = false;

		for (int i = 0; i < totalBlocks; i++)
		{
			// find the next block that hasn't been loaded as part of a file yet
			if (blocks & (1 << i)) continue;

			const unsigned blockStart = i << 17;

			MemPackItem newItem;
			if (newItem.tryLoadFrom(file, blockStart + 0x7fb0)
				|| newItem.tryLoadFrom(file, blockStart + 0xffb0))
			{
				// issue some kind of warning if a file's block allocation overlaps another file
				// (e.g. the dump of kirby guruguru ball in no-intro which has dupe files w/ wrong blocks)
				if (blocks & newItem.header.blocks)
				{
					recoveryWarning = true;
				}
				
				newItems.append(newItem);

				blocks |= MemPackItem::normalizeBlocks(newItem.header.blocks, file.size());
			}
		}
		// TODO: try to show placeholder entries for non-empty leftover 'junk' blocks

		if (recoveryWarning)
		{
			QMessageBox::warning(this, tr("Open File"),
				tr("At least one file in this memory pack seems to have been partially overwritten.\n\n"
				   "Fully recovering all files may not be possible."));
		}
		else if (newItems.count() == 0)
		{
			QMessageBox::warning(this, tr("Open File"),
				tr("No valid BS-X files were found in this memory pack."));
		}
	}

	return newItems;
}
