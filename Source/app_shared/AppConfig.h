#pragma once

#include "Config.h"
#include "Singleton.h"

class CAppConfig : public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
	CAppConfig();
	virtual ~CAppConfig() = default;

	//This needs to be implemented by every application/executable
	fs::path GetBasePath() const;
	
private:
	CConfig::PathType BuildConfigPath();
};
