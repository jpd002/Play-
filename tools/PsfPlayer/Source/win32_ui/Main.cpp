#include "PsfVm.h"
#include "PsfLoader.h"
#include "MainWindow.h"
#include "MiniDebugger.h"
#include <boost/filesystem/path.hpp>

namespace filesystem = boost::filesystem;

int WINAPI WinMain(HINSTANCE, HINSTANCE, char* commandLine, int)
{
	CPsfVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		if(strlen(commandLine) == 0)
		{
			MessageBox(NULL, _T("Please provide a PSF file path as a command line argument."), NULL, 16);
			return 1;
		}
		virtualMachine.Reset();
		filesystem::path loadPath(commandLine);
		CPsfLoader::LoadPsf(virtualMachine, loadPath, filesystem::path());
		std::string tagPackageName = loadPath.leaf().string().c_str();
		virtualMachine.LoadDebugTags(tagPackageName.c_str());
		CMiniDebugger debugger(virtualMachine, virtualMachine.GetDebugInfo());
		debugger.Show(SW_SHOW);
		debugger.Run();
		virtualMachine.SaveDebugTags(tagPackageName.c_str());
	}
#else
	{
		CMainWindow window(virtualMachine);
		window.Center();
		window.Show(SW_SHOW);
		window.Run();
//		CPlayerWnd player(virtualMachine);
//		player.Center();
//		player.Show(SW_SHOW);
//		player.Run();
	}
#endif

	return 0;
}
