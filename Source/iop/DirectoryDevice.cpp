#include "DirectoryDevice.h"
#include "StdStream.h"
#include "../Config.h"

using namespace Framework;
using namespace std;

CDirectoryDevice::CDirectoryDevice(const char* basePathPreferenceName) :
m_basePathPreferenceName(basePathPreferenceName)
{

}

CDirectoryDevice::~CDirectoryDevice()
{

}

CStream* CDirectoryDevice::GetFile(uint32 accessType, const char* devicePath)
{
	const char* mode(NULL);
    string path;

	const char* basePath = CConfig::GetInstance()->GetPreferenceString(m_basePathPreferenceName.c_str());

    path = basePath;
	if(devicePath[0] != '/')
	{
        path += "/";
	}
    path += devicePath;

	switch(accessType)
	{
	case 0:
	case O_RDONLY:
		mode = "rb";
		break;
	case (O_RDWR | O_CREAT):
		mode = "w+";
		break;
	default:
		assert(0);
		break;
	}


    FILE* stream = fopen(path.c_str(), mode);
	if(stream == NULL) return NULL;

	return new CStdStream(stream);
}
