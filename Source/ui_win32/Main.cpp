#include "MainWindow.h"

#ifdef VTUNE_ENABLED
#include <jitprofiling.h>
#endif

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	CPS2VM virtualMachine;
	CMainWindow MainWindow(virtualMachine);
	MainWindow.Loop();
#ifdef VTUNE_ENABLED
	iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, NULL);
#endif
	return 0;
}
