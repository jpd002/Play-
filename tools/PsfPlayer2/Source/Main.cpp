#include "PsfBase.h"
#include "StdStream.h"
#include "PsxVm.h"
#include "MiniDebugger.h"
#include "PlayerWnd.h"

using namespace Framework;

int main(int argc, char** argv)
{
	CStdStream input("C:\\Media\\LoM_psf\\102 Nostalgic song.PSF", "rb");
	CPsfBase psfFile(input);
	CPsxVm virtualMachine;
	virtualMachine.Reset();
	virtualMachine.LoadExe(psfFile.GetProgram());

//    CMiniDebugger debugger(virtualMachine);
//    debugger.Show(SW_SHOW);
//    debugger.Run();

	CPlayerWnd player(virtualMachine);
	player.Center();
	player.Show(SW_SHOW);
	Win32::CWindow::StdMsgLoop(&player);
	return 0;
}
