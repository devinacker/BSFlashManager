#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"

#include "mempackitem.h"

class MemPackModel;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);

protected:
	void closeEvent(QCloseEvent*);

private slots:
	void about();

	void newFile();
	void openFile();
	bool saveFile();
	bool saveFileAs();

	void exportSelected();
	void exportAll();

	void transferTest();

	void updateSelected();
	void applyChanges();

	void addFiles();
	void deleteFile();
	void moveFileUp();
	void moveFileDown();

private:
	bool promptSave();
	bool openFile(const QString&);
	bool saveFile(const QString&);
	void updateWindowTitle();
	void updateBlockCount();

	MemPackItems loadAllItems(const QString& path);

	MemPackModel *memPackModel;

	QString lastFileName;
	Ui::MainWindow ui;
};
