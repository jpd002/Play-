#include "PsxVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MiniDebugger.h"

using namespace Framework;
using namespace Psx;

//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE, HINSTANCE, char*, int)
{
//	CPsxVm virtualMachine;
    PS2::CPsfVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\C-SotN_psf\\Master Librarian.minipsf");
        CPsfLoader::LoadPsf2(virtualMachine, "C:\\Media\\PS2\\FF4\\ff4-01.psf2");

		CMiniDebugger debugger(virtualMachine, virtualMachine.GetIopDebug());
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
