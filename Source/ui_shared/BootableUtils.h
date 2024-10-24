#pragma once

#include <string>
#include "filesystem_def.h"

namespace BootableUtils
{
	enum BOOTABLE_TYPE
	{
		UNKNOWN = 0,
		PS2_DISC = 1 << 0,
		PS2_ARCADE = 1 << 1,
		PS2_ELF = 1 << 2,
	};

	bool TryGetDiskId(const fs::path&, std::string*);

	bool IsBootableExecutablePath(const fs::path&);
	bool IsBootableDiscImagePath(const fs::path&);
	bool IsBootableArcadeDefPath(const fs::path&);

	BOOTABLE_TYPE GetBootableType(const fs::path&);
}
