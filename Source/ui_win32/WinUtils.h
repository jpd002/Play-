#pragma once

#include "win32/Window.h"
#include "Types.h"

namespace WinUtils
{
	TCHAR						FixSlashes(TCHAR);
	HBITMAP						CreateMask(HBITMAP, uint32);
};
