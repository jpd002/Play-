#pragma once

#include <string>
#include <memory>
#include <boost/filesystem.hpp>
#include "ISO9660/ISO9660.h"

namespace DiskUtils
{
	typedef std::unique_ptr<CISO9660> Iso9660Ptr;
	typedef std::map<std::string, std::string> SystemConfigMap;

	DiskUtils::Iso9660Ptr	CreateDiskImageFromPath(const boost::filesystem::path&);
	SystemConfigMap			ParseSystemConfigFile(Framework::CStream*);

	bool					TryGetDiskId(const boost::filesystem::path&, std::string*);
}
