#include <windows.h>
#include "..\SH_WaveOut.h"

int WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
	return TRUE;
}

extern "C"
{
	__declspec(dllexport) CSoundHandler* HandlerFactory()
	{
		return new CSH_WaveOut();
	}
}
