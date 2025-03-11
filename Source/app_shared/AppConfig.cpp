#include "AppConfig.h"
#include "PathUtils.h"

#define CONFIG_FILENAME ("config.xml")

CAppConfig::CAppConfig()
    : CConfig(BuildConfigPath())
{
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	return GetBasePath() / CONFIG_FILENAME;
}
