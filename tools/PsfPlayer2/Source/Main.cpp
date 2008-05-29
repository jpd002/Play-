#include "PsxVm.h"
#include "PlayerWnd.h"

using namespace Framework;

int main(int argc, char** argv)
{
	CPsxVm virtualMachine;

//    CMiniDebugger debugger(virtualMachine);
//    debugger.Show(SW_SHOW);
//    debugger.Run();

	CPlayerWnd player(virtualMachine);
	player.Center();
	player.Show(SW_SHOW);
	Win32::CWindow::StdMsgLoop(&player);
	return 0;
}
