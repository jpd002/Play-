#pragma once

#include <string>
#include <memory>
#include "filesystem_def.h"
#include "OpticalMedia.h"

namespace DiskUtils
{
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;
	typedef std::map<std::string, std::string> SystemConfigMap;

	OpticalMediaPtr CreateOpticalMediaFromPath(const fs::path&, uint32 = 0);
	SystemConfigMap ParseSystemConfigFile(Framework::CStream*);

	bool TryGetDiskId(const fs::path&, std::string*);
}
