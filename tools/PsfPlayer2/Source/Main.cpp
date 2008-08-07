#include "PsxVm.h"
#include "PsfLoader.h"
#include "PlayerWnd.h"
#include "MiniDebugger.h"

using namespace Framework;
using namespace Psx;

int main(int argc, char** argv)
{
	CPsxVm virtualMachine;

#ifdef DEBUGGER_INCLUDED
	{
		virtualMachine.Reset();
		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\FF7_psf\\FF7 408 Hurry Faster!.minipsf");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\vp-psf\\vp-201.minipsf");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\Chrono Cross\\Chrono Cross 102 The Brink of Death.psf");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\Xenogears_psf\\107 Steel Giant.PSF");
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\ToP_psf\\top 214 castle of the dhaos.minipsf");
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
