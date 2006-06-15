#ifndef _UTILS_H_
#define _UTILS_H_

#include "Stream.h"
#include "Str.h"
#include "win32/Window.h"

namespace Utils
{
	void			GetLine(Framework::CStream*, Framework::CStrA*, bool = true);
	const char*		GetFilenameFromPath(const char*, char = '\\');
};

#endif
