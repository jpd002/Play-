#include "AppConfig.h"
#include "PathUtils.h"

#define BASE_DATA_PATH ("McServTest Data Files")
#define CONFIG_FILENAME ("config.xml")

CAppConfig::CAppConfig()
    : CConfig(BuildConfigPath())
{
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	return GetBasePath() / CONFIG_FILENAME;
}

CAppConfigBasePath::CAppConfigBasePath()
{
	m_basePath = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
	Framework::PathUtils::EnsurePathExists(m_basePath);
}

fs::path CAppConfigBasePath::GetBasePath() const
{
	return m_basePath;
}
