#pragma once

#include "filesystem_def.h"

namespace Iop
{
	namespace PathUtils
	{
		fs::path MakeHostPath(const fs::path&, const char*);
		bool IsInsideBasePath(const fs::path&, const fs::path&);
	}
}
