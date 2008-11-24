#include "PsfVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
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
		filesystem::path loadPath("C:\\Media\\PSX\\vp-psf\\vp-103.minipsf", filesystem::native);
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
		CPlayerWnd player(virtualMachine);
		player.Center();
		player.Show(SW_SHOW);
		player.Run();
	}
#endif

	return 0;
}
