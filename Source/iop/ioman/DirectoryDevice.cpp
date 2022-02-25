#include <cassert>
#include "DirectoryDevice.h"
#include "../Iop_PathUtils.h"
#include "PathDirectoryIterator.h"
#include "StdStream.h"
#include "string_cast.h"

using namespace Iop::Ioman;

template <typename StringType>
Framework::CStdStream* CreateStdStream(const StringType&, const char*);

template <>
Framework::CStdStream* CreateStdStream(const std::string& path, const char* mode)
{
	return new Framework::CStdStream(path.c_str(), mode);
}

template <>
Framework::CStdStream* CreateStdStream(const std::wstring& path, const char* mode)
{
	auto cvtMode = string_cast<std::wstring>(mode);
	return new Framework::CStdStream(path.c_str(), cvtMode.c_str());
}

Framework::CStream* CDirectoryDevice::GetFile(uint32 accessType, const char* devicePath)
{
	auto basePath = GetBasePath();
	auto path = Iop::PathUtils::MakeHostPath(basePath, devicePath);

	//Get rid of unwanted flags
	//Used by Midnight Club 3
	accessType &= ~OPEN_FLAG_NOWAIT;

	const char* mode = nullptr;
	switch(accessType)
	{
	default:
		assert(0);
		[[fallthrough]];
	case 0:
	case OPEN_FLAG_RDONLY:
		mode = "rb";
		break;
	case(OPEN_FLAG_WRONLY | OPEN_FLAG_CREAT):
	case(OPEN_FLAG_WRONLY | OPEN_FLAG_CREAT | OPEN_FLAG_TRUNC):
		mode = "wb";
		break;
	case OPEN_FLAG_WRONLY:
	case OPEN_FLAG_RDWR:
		mode = "r+b";
		break;
	case(OPEN_FLAG_RDWR | OPEN_FLAG_CREAT):
	case(OPEN_FLAG_RDWR | OPEN_FLAG_CREAT | OPEN_FLAG_TRUNC):
		mode = "w+b";
		break;
	}

	try
	{
		return CreateStdStream(path.native(), mode);
	}
	catch(...)
	{
		return nullptr;
	}
}

DirectoryIteratorPtr CDirectoryDevice::GetDirectory(const char* devicePath)
{
	auto basePath = GetBasePath();
	auto path = Iop::PathUtils::MakeHostPath(basePath, devicePath);
	if(!fs::is_directory(path))
	{
		throw std::runtime_error("Not a directory.");
	}
	return std::make_unique<CPathDirectoryIterator>(path);
}

void CDirectoryDevice::MakeDirectory(const char* devicePath)
{
	auto basePath = GetBasePath();
	auto path = Iop::PathUtils::MakeHostPath(basePath, devicePath);
	if(!fs::create_directory(path))
	{
		throw std::runtime_error("Failed to create directory.");
	}
}
