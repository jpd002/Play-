#include <windows.h>
#include "..\PsfVm.h"
#include "MiniDebugger.h"
#include <io.h>
#include <fcntl.h>

using namespace std;

void InitializeConsole()
{
	AllocConsole();

	CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBufferInfo);
	ScreenBufferInfo.dwSize.Y = 1000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), ScreenBufferInfo.dwSize);

	(*stdout) = *_fdopen(_open_osfhandle(
		reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)),
		_O_TEXT), "w");

	setvbuf(stdout, NULL, _IONBF, 0);
	ios::sync_with_stdio();	
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, char*, int)
{
//    string filename = "C:\\nsf\\kh\\Kingdom Hearts (Sequences Only)\\101 - Dearly Beloved.psf2";
//    string filename = "C:\\nsf\\FF10\\111 Game Over.minipsf2";
    string filename = "C:\\Projects\\Purei\\tools\\PsfPlayer\\226 Castle Zvahl.psf2";
    InitializeConsole();
    CPsfVm virtualMachine(filename.c_str());
	
    CMiniDebugger debugger(virtualMachine);
    debugger.Show(SW_SHOW);
    debugger.Run();
    return 1;
}
