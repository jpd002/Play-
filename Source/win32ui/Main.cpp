#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, char* sCmdLine, int)
{
	CPS2VM virtualMachine;
	CMainWindow MainWindow(virtualMachine, sCmdLine);
	return MainWindow.Loop();
}
