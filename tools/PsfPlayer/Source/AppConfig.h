#ifndef _APPCONFIG_H_
#define _APPCONFIG_H_

#include "Config.h"
#include "Singleton.h"

class CAppConfig : public Framework::CConfig, public CSingleton<CAppConfig>
{
public:
    friend class CSingleton<CAppConfig>;

    static CConfig::PathType    GetBasePath();

    static CConfig::PathType    Utf8ToPath(const char*);
    static std::string          PathToUtf8(const CConfig::PathType&);

private:
                                CAppConfig();
    virtual                     ~CAppConfig();

    static CConfig::PathType    BuildConfigPath();
};

#endif
