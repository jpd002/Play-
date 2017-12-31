#include "DirectoryDevice.h"
#include "StdStream.h"
#include "string_cast.h"
#include "../AppConfig.h"

using namespace Iop::Ioman;

CDirectoryDevice::CDirectoryDevice(const char* basePathPreferenceName)
: m_basePathPreferenceName(basePathPreferenceName)
{

}

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
	auto basePath = CAppConfig::GetInstance().GetPreferencePath(m_basePathPreferenceName.c_str());
	auto path = basePath / devicePath;

	const char* mode = nullptr;
	switch(accessType)
	{
	case 0:
	case OPEN_FLAG_RDONLY:
		mode = "rb";
		break;
	case (OPEN_FLAG_RDWR | OPEN_FLAG_CREAT):
		mode = "w+";
		break;
	default:
		assert(0);
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
