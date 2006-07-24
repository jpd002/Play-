#ifndef _WINUTILS_H_
#define _WINUTILS_H_

#include "win32/Window.h"
#include "Types.h"

namespace WinUtils
{
	TCHAR			FixSlashes(TCHAR);
	HBITMAP			CreateMask(HBITMAP, uint32);
};

#endif
