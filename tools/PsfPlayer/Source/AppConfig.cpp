#include "AppConfig.h"
#include "PathUtils.h"
#include "Utf8.h"
#if !defined(WIN32)
#include <pwd.h>
#endif

#define BASE_DATA_PATH          L"PsfPlayer Data Files"
#define DEFAULT_CONFIG_PATH     (L"config.xml")

CAppConfig::CAppConfig() :
CConfig(BuildConfigPath())
{

}

CAppConfig::~CAppConfig()
{

}

Framework::CConfig::PathType CAppConfig::GetBasePath()
{
#if defined(WIN32)
	return (Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH);
#elif defined(MACOSX)
	return (Utf8ToPath(Framework::PathUtils::GetHomePath().string().c_str()) / BASE_DATA_PATH);
#else
	return CConfig::PathType();
#endif
}

Framework::CConfig::PathType CAppConfig::Utf8ToPath(const char* path)
{
	return CConfig::PathType(Framework::Utf8::ConvertFrom(path));
}

std::string CAppConfig::PathToUtf8(const CConfig::PathType& path)
{
	return Framework::Utf8::ConvertTo(path.string());
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
#if defined(MACOSX)
	passwd* userInfo = getpwuid(getuid());
	if(userInfo == NULL) return DEFAULT_CONFIG_PATH;
	return wstring(Utf8::ConvertFrom(userInfo->pw_dir)) + L"/Library/Preferences/com.vapps.Purei.xml";
#elif defined(WIN32)
    CConfig::PathType userPath(GetBasePath());
	Framework::PathUtils::EnsurePathExists(userPath);
    return (userPath / L"config.xml");
#else
	return DEFAULT_CONFIG_PATH;
#endif
}
