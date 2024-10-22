#pragma once

#include <string>
#include <memory>
#include <set>
#include "filesystem_def.h"
#include "OpticalMedia.h"

namespace DiskUtils
{
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;
	typedef std::map<std::string, std::string> SystemConfigMap;
	typedef std::set<std::string> ExtensionList;

	const ExtensionList& GetSupportedExtensions();

	OpticalMediaPtr CreateOpticalMediaFromPath(const fs::path&, uint32 = 0);
	SystemConfigMap ParseSystemConfigFile(Framework::CStream*);

	bool TryGetDiskId(const fs::path&, std::string*);
}
