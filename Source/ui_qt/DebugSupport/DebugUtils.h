#ifndef _DEBUGUTILS_H_
#define _DEBUGUTILS_H_

// #include "tcharx.h"
// #include <basic_string>
#include <string>
#include <cstring>
#include <iostream>
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"

namespace DebugUtils
{
	std::string PrintAddressLocation(uint32, CMIPS*, const BiosDebugModuleInfoArray&);
	const BIOS_DEBUG_MODULE_INFO* FindModuleAtAddress(const BiosDebugModuleInfoArray&, uint32);
}

#endif
