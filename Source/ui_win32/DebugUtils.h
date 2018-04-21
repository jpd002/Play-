#ifndef _DEBUGUTILS_H_
#define _DEBUGUTILS_H_

#include "../BiosDebugInfoProvider.h"
#include "../MIPS.h"
#include "tcharx.h"

namespace DebugUtils
{
std::tstring                  PrintAddressLocation(uint32, CMIPS*, const BiosDebugModuleInfoArray&);
const BIOS_DEBUG_MODULE_INFO* FindModuleAtAddress(const BiosDebugModuleInfoArray&, uint32);
}

#endif
