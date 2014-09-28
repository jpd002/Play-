#pragma once

#include "win32/Window.h"
#include "Types.h"

namespace WinUtils
{
	TCHAR						FixSlashes(TCHAR);
	HBITMAP						CreateMask(HBITMAP, uint32);
	int							PointsToPixels(int);
	Framework::Win32::CRect		PointsToPixels(const Framework::Win32::CRect&);
};
