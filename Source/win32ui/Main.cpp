#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, char* sCmdLine, int)
{
	CMainWindow MainWindow(sCmdLine);
	return MainWindow.Loop();
}
