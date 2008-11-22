#include "PsfVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MiniDebugger.h"

using namespace Framework;

//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE, HINSTANCE, char*, int)
{
    CPsfVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PS2\\FFXI_psf2\\FFXI_psf2\\104 Ronfaure.psf2");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PS2\\FF4\\ff4-01.psf2");
		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PS2\\FF10_psf\\102 In Zanarkand.minipsf2");
		CMiniDebugger debugger(virtualMachine, virtualMachine.GetDebugInfo());
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
