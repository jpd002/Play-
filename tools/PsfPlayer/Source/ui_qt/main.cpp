#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	QApplication a(argc, argv);

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption psf_option("psf", "Load specified PSF file", "psf");
	parser.addOption(psf_option);

	parser.process(a);

	MainWindow w;
	w.show();

	if(parser.isSet(psf_option))
	{
		QString psf = parser.value(psf_option);
		w.AddFileToPlaylist(psf.toStdString());
	}

	return a.exec();
}
