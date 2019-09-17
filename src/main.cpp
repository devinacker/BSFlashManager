#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationDisplayName("BS-X Flash Manager");
	a.setApplicationName(a.applicationDisplayName());
	a.setApplicationVersion("0.1.0");

	MainWindow w;
	w.show();
	return a.exec();
}
