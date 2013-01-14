#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	CMainWindow mainWindow;
	mainWindow.Center();
	mainWindow.Show(SW_SHOW);
	Framework::Win32::CWindow::StdMsgLoop(mainWindow);
	return 0;
}
