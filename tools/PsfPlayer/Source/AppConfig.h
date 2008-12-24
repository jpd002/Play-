#ifndef _APPCONFIG_H_
#define _APPCONFIG_H_

#include "Config.h"
#include "Singleton.h"

class CAppConfig : public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
    friend class CSingleton<CAppConfig>;

private:
                                CAppConfig();
    virtual                     ~CAppConfig();
};

#endif
