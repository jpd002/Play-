#include "PsfVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MainWindow.h"
#include "MiniDebugger.h"
#include <boost/filesystem/path.hpp>

using namespace Framework;
using namespace std;
namespace filesystem = boost::filesystem;

//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE, HINSTANCE, char*, int)
{
    CPsfVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
//		filesystem::path loadPath("C:\\Media\\PS2\\Ys4_psf2\\13 - Blazing Sword.psf2", filesystem::native);
//		filesystem::path loadPath("C:\\Media\\PSX\\vp-psf\\vp-103.minipsf", filesystem::native);
		filesystem::path loadPath("D:\\Media\\PS2\\FFX\\102 In Zanarkand.minipsf2", filesystem::native);
//        filesystem::path loadPath("D:\\Media\\PS2\\Kingdom Hearts\\Kingdom Hearts (Sequences Only)\\101 - Dearly Beloved.psf2");
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
		Win32::CWindow::StdMsgLoop(&window);
//		CPlayerWnd player(virtualMachine);
//		player.Center();
//		player.Show(SW_SHOW);
//		player.Run();
	}
#endif

	return 0;
}
