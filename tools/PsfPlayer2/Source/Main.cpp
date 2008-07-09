#include "PsxVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MiniDebugger.h"

using namespace Framework;
using namespace Psx;

int main(int argc, char** argv)
{
	CPsxVm virtualMachine;

#ifdef _DEBUG
	{
		virtualMachine.Reset();
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\FF7_psf\\FF7 408 Hurry Faster!.minipsf");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\vp-psf\\vp-201.minipsf");
		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\Chrono Cross\\Chrono Cross 102 The Brink of Death.psf");
		CMiniDebugger debugger(virtualMachine);
		debugger.Show(SW_SHOW);
		debugger.Run();
	}
#else
	{
		CPlayerWnd player(virtualMachine);
		player.Center();
		player.Show(SW_SHOW);
		Win32::CWindow::StdMsgLoop(&player);
	}
#endif

	return 0;
}
