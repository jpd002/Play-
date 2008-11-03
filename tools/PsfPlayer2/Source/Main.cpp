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
//		CPsfLoader::LoadPsf(virtualMachine, "C:\\Media\\PSX\\C-SotN_psf\\Master Librarian.minipsf");
//        CPsfLoader::LoadPsf2(virtualMachine, "D:\\Media\\PS2\\FF4\\ff4-02.psf2");
        CPsfLoader::LoadPsf(virtualMachine, "C:\\Downloads\\vp-psf\\vp-114.minipsf");

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
