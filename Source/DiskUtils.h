#pragma once

#include "OpticalMedia.h"
#include <boost/filesystem.hpp>
#include <memory>
#include <string>

namespace DiskUtils
{
typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;
typedef std::map<std::string, std::string> SystemConfigMap;

OpticalMediaPtr CreateOpticalMediaFromPath(const boost::filesystem::path&);
SystemConfigMap ParseSystemConfigFile(Framework::CStream*);

bool TryGetDiskId(const boost::filesystem::path&, std::string*);
}
