#ifndef _APPCONFIG_H_
#define _APPCONFIG_H_

#include "Config.h"
#include "Singleton.h"

class CAppConfig : public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
	friend class CSingleton<CAppConfig>;

	static CConfig::PathType	GetBasePath();

private:
								CAppConfig();
	virtual						~CAppConfig();

	static CConfig::PathType	BuildConfigPath();
};

#endif
