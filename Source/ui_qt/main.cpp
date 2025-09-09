#include <QApplication>
#include <QCommandLineParser>
#include <QtGlobal>
#include "mainwindow.h"
#include "PathUtils.h"
#include "QStringUtils.h"

Q_DECLARE_METATYPE(std::string)

#ifdef WIN32
#ifdef TEKNOPARROT
void InitializeEscapeKeyMonitor()
{
	static bool g_escape_monitor_initialized = false;
	static HANDLE g_escape_thread = nullptr;

	// Only initialize once
	if(g_escape_monitor_initialized)
		return;

	g_escape_monitor_initialized = true;

	// Create the monitoring thread
	g_escape_thread = CreateThread(
	    nullptr,              // Default security attributes
	    0,                    // Default stack size
	    [](LPVOID) -> DWORD { // Thread function
		    while(true)
		    {
			    // Check if ESC key is pressed (virtual key code 0x1B)
			    if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			    {
				    // ESC is pressed, exit the process immediately
				    ExitProcess(0);
			    }

			    // Small delay to prevent excessive CPU usage
			    Sleep(10);
		    }
		    return 0;
	    },
	    nullptr, // No parameter to thread function
	    0,       // Default creation flags
	    nullptr  // Don't need thread ID
	);

	if(g_escape_thread)
	{
		// Set thread priority to below normal so it doesn't interfere with main execution
		SetThreadPriority(g_escape_thread, THREAD_PRIORITY_BELOW_NORMAL);
	}
}
#endif
#endif

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	QApplication a(argc, argv);

	QCoreApplication::setApplicationName("Play!");
	QCoreApplication::setApplicationVersion("Version: " PLAY_VERSION);

	qRegisterMetaType<std::string>();
#ifdef WIN32
#ifdef TEKNOPARROT
	// Initialize ESC key monitor for immediate exit
	InitializeEscapeKeyMonitor();
#endif
#endif

	QCommandLineParser parser;
	parser.setApplicationDescription("Description: A multiplatform PS2 emulator.");
	parser.addHelpOption();
	parser.addVersionOption();

#ifdef DEBUGGER_INCLUDED
	QCommandLineOption debugger_option("debugger", "Show debugger");
	parser.addOption(debugger_option);
	QCommandLineOption frame_debugger_option("framedebugger", "Show frame debugger");
	parser.addOption(frame_debugger_option);
#endif

	QCommandLineOption fullscreen_option("fullscreen", "Start the emulator fullscreen.");
	parser.addOption(fullscreen_option);

	QCommandLineOption cdrom_image_option("cdrom0", "Boot last booted cdvd image");
	parser.addOption(cdrom_image_option);

	QCommandLineOption disc_image_option("disc", "Boot any supported disc image", "disc_image");
	parser.addOption(disc_image_option);

	QCommandLineOption elf_image_option("elf", "Boot supported elf image", "elf_image");
	parser.addOption(elf_image_option);

	QCommandLineOption arcade_id_option("arcade", "Boot arcade game with specified id", "arcade_id");
	parser.addOption(arcade_id_option);

	QCommandLineOption load_state_option("state", "Load state at index", "state_index");
	parser.addOption(load_state_option);

	parser.process(a);

	MainWindow w;
	w.show();

	if(parser.isSet(cdrom_image_option))
	{
		try
		{
			w.BootCDROM();
		}
		catch(const std::exception& e)
		{
			printf("Error: %s\r\n", e.what());
		}
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
	else if(parser.isSet(arcade_id_option))
	{
		QString arcade_id = parser.value(arcade_id_option);
		w.BootArcadeMachine(arcade_id.toStdString() + ".arcadedef");
	}

	if(parser.isSet(fullscreen_option))
	{
		w.showFullScreen();
	}

	if(parser.isSet(load_state_option))
	{
		QString stateIndex = parser.value(load_state_option);
		w.loadState(stateIndex.toInt());
	}

#ifdef DEBUGGER_INCLUDED
	if(parser.isSet(debugger_option))
	{
		w.ShowDebugger();
	}
	if(parser.isSet(frame_debugger_option))
	{
		w.ShowFrameDebugger();
	}
#endif
	return a.exec();
}

fs::path CAppConfig::GetBasePath() const
{
	static const char* BASE_DATA_PATH = "Play Data Files";
	static const auto basePath =
	    []() {
		    fs::path result;
		    if(fs::exists("portable.txt"))
		    {
			    result = BASE_DATA_PATH;
		    }
		    else
		    {
			    result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
		    }
		    Framework::PathUtils::EnsurePathExists(result);
		    return result;
	    }();
	return basePath;
}
