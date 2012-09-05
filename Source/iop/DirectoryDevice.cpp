#include "DirectoryDevice.h"
#include "StdStream.h"
#include "../AppConfig.h"

using namespace Iop::Ioman;

CDirectoryDevice::CDirectoryDevice(const char* basePathPreferenceName)
: m_basePathPreferenceName(basePathPreferenceName)
{

}

CDirectoryDevice::~CDirectoryDevice()
{

}

Framework::CStream* CDirectoryDevice::GetFile(uint32 accessType, const char* devicePath)
{
	const char* mode(NULL);
	std::string path;

	const char* basePath = CAppConfig::GetInstance().GetPreferenceString(m_basePathPreferenceName.c_str());

	path = basePath;
	if(devicePath[0] != '/')
	{
		path += "/";
	}
	path += devicePath;

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


	FILE* stream = fopen(path.c_str(), mode);
	if(stream == NULL) return NULL;

	return new Framework::CStdStream(stream);
}
