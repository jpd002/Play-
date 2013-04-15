#include <windows.h>
#include "..\SH_XAudio2.h"

int WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
	return TRUE;
}

extern "C"
{
	__declspec(dllexport) CSoundHandler* HandlerFactory()
	{
		return new CSH_XAudio2();
	}
}
