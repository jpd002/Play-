#include "MainWindow.h"

#ifdef VTUNE_ENABLED
#include <jitprofiling.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, char* sCmdLine, int)
{
	CPS2VM virtualMachine;
	CMainWindow MainWindow(virtualMachine, sCmdLine);
	MainWindow.Loop();
#ifdef VTUNE_ENABLED
	iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, NULL);
#endif
	return 0;
}
