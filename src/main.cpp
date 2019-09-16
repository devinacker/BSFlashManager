#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationDisplayName("BS-X Flash Manager");

	MainWindow w;
	w.show();
	return a.exec();
}
