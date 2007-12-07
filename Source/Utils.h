#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include "Stream.h"

namespace Utils
{
	void			GetLine(Framework::CStream*, std::string*, bool = true);
	const char*		GetFilenameFromPath(const char*, char = '\\');
};

#endif
