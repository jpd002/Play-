#include "PsxVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MiniDebugger.h"

using namespace Framework;
using namespace Psx;

//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE, HINSTANCE, char*, int)
{
	CPsxVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\C-SotN_psf\\Master Librarian.minipsf");
		CMiniDebugger debugger(virtualMachine);
		debugger.Show(SW_SHOW);
		debugger.Run();
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
