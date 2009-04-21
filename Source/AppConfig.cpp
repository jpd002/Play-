#include "AppConfig.h"
#include "PathUtils.h"
#include "Utf8.h"

#define BASE_DATA_PATH          L"Purei Data Files"
#define DEFAULT_CONFIG_PATH     ("config.xml")

using namespace Framework;
using namespace boost;
using namespace std;

CAppConfig::CAppConfig() :
CConfig(BuildConfigPath())
{

}

CAppConfig::~CAppConfig()
{

}

CConfig::PathType CAppConfig::GetBasePath()
{
    return (PathUtils::GetPersonalDataPath() / BASE_DATA_PATH);
}

CConfig::PathType CAppConfig::Utf8ToPath(const char* path)
{
#if defined(WIN32)
    return CConfig::PathType(Utf8::ConvertFrom(path));
#else
    return CConfig::PathType(path)
#endif
}

string CAppConfig::PathToUtf8(const CConfig::PathType& path)
{
#if defined(WIN32)
    return Utf8::ConvertTo(path.string());
#else
    return path.string();
#endif
}

CConfig::PathType CAppConfig::BuildConfigPath()
{
#if defined(MACOSX)
	passwd* userInfo = getpwuid(getuid());
	if(userInfo == NULL) return DEFAULT_CONFIG_PATH;
	return string(userInfo->pw_dir) + "/Library/Preferences/com.vapps.Purei.xml";
#elif defined(WIN32)
    CConfig::PathType userPath(GetBasePath());
    PathUtils::EnsurePathExists(userPath);
    return (userPath / L"Config.xml");
#else
	return DEFAULT_CONFIG_PATH;
#endif
}
