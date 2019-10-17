#include "AppConfig.h"
#include "PathUtils.h"

#define BASE_DATA_PATH ("Play Data Files")
#define CONFIG_FILENAME ("config.xml")

CAppConfig::CAppConfig()
    : CConfig(BuildConfigPath())
{
}

Framework::CConfig::PathType CAppConfig::GetBasePath()
{
	auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
	return result;
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	auto userPath(GetBasePath());
	Framework::PathUtils::EnsurePathExists(userPath);
	return userPath / CONFIG_FILENAME;
}
