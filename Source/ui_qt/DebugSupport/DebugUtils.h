#pragma once

#include <string>
#include <vector>
#include <QFont>
#include "BiosDebugInfoProvider.h"

class CMIPS;

namespace DebugUtils
{
	std::string PrintAddressLocation(uint32, CMIPS*, const BiosDebugModuleInfoArray&);
	const BIOS_DEBUG_MODULE_INFO* FindModuleAtAddress(const BiosDebugModuleInfoArray&, uint32);
	std::vector<uint32> FindCallers(CMIPS*, uint32);
	std::vector<uint32> FindWordValueRefs(CMIPS*, uint32, uint32);
	QFont CreateMonospaceFont();
}
