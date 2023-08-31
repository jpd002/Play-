#pragma once

#include "Config.h"
#include "Singleton.h"

class CAppConfigBasePath
{
public:
	CAppConfigBasePath();
	fs::path GetBasePath() const;

private:
	fs::path m_basePath;
};

class CAppConfig : public CAppConfigBasePath, public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
	CAppConfig();
	virtual ~CAppConfig() = default;

private:
	CConfig::PathType BuildConfigPath();
};
