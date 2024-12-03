#include <QApplication>
#include "MainWindow.h"

int main(int argc, char** argv)
{
	QApplication a(argc, argv);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	CMainWindow w;
	w.show();
	return a.exec();
}
