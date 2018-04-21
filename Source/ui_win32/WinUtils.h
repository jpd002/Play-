#pragma once

#include "Types.h"
#include "win32/Window.h"

namespace WinUtils
{
TCHAR   FixSlashes(TCHAR);
HBITMAP CreateMask(HBITMAP, uint32);
void    CopyStringToClipboard(const std::tstring&);
};
