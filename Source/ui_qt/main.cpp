#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.h"
#include "QStringUtils.h"

int main(int argc, char* argv[])
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication a(argc, argv);

	QCoreApplication::setApplicationName("Play!");
	QCoreApplication::setApplicationVersion("Version: " PLAY_VERSION);

	QCommandLineParser parser;
	parser.setApplicationDescription("Description: A multiplatform PS2 emulator.");
	parser.addHelpOption();
	parser.addVersionOption();

#ifdef DEBUGGER_INCLUDED
	QCommandLineOption debugger_option("debugger", "Show debugger");
	parser.addOption(debugger_option);
#endif
	QCommandLineOption cdrom_image_option("cdrom0", "Boot last booted cdvd image");
	parser.addOption(cdrom_image_option);
	QCommandLineOption disc_image_option("disc", "Boot any supported disc image", "disc_image");
	parser.addOption(disc_image_option);
	QCommandLineOption elf_image_option("elf", "Boot supported elf image", "elf_image");
	parser.addOption(elf_image_option);
	parser.process(a);

	MainWindow w;
#ifdef DEBUGGER_INCLUDED
	a.installNativeEventFilter(&w);
#endif
	w.show();

	if(parser.isSet(cdrom_image_option))
	{
		w.BootCDROM();
	}
	else if(parser.isSet(disc_image_option))
	{
		QString disc_image = parser.value(disc_image_option);
		w.LoadCDROM(QStringToPath(disc_image));
		w.BootCDROM();
	}
	else if(parser.isSet(elf_image_option))
	{
		QString elf_image = parser.value(elf_image_option);
		w.BootElf(QStringToPath(elf_image));
	}

#ifdef DEBUGGER_INCLUDED
	if(parser.isSet(debugger_option))
	{
		w.ShowDebugger();
	}
#endif
	return a.exec();
}
