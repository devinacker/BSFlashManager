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

private slots:
	void openFile();
	void saveFile();
	void saveFileAs();

	void exportSelected();
	void exportAll();
	
	void updateSelected();
	void applyChanges();

	void addFiles();
	void deleteFile();
	void moveFileUp();
	void moveFileDown();

private:
	void saveFile(const QString&);
	void updateWindowTitle();
	void updateBlockCount();

	MemPackItems loadAllItems(const QString& path);

	MemPackModel *memPackModel;

	QString lastFileName;
	Ui::MainWindow ui;
};
