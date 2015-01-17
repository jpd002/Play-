#pragma once

#include "Config.h"
#include "Singleton.h"

class CAppConfig : public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
								CAppConfig();
	virtual						~CAppConfig();

	static CConfig::PathType	GetBasePath();

private:
	static CConfig::PathType	BuildConfigPath();
};
