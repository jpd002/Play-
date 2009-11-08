#include <windows.h>
#include "..\..\SH_OpenAL.h"

int WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
	return TRUE;
}

extern "C"
{
	__declspec(dllexport) CSoundHandler* HandlerFactory()
	{
		return new CSH_OpenAL();
	}
}
