#include "PsfVm.h"
#include "PsfLoader.h"
#include "MainWindow.h"
#include "MiniDebugger.h"
#include <boost/filesystem/path.hpp>

using namespace Framework;
using namespace std;
namespace filesystem = boost::filesystem;

//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE, HINSTANCE, char* commandLine, int)
{
    CPsfVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
		filesystem::path loadPath(commandLine);
		CPsfLoader::LoadPsf(virtualMachine, loadPath.string().c_str());
		string tagPackageName = loadPath.leaf();
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
