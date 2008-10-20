#include "AppConfig.h"

#define DEFAULT_CONFIG_PATH     L"config.xml"

using namespace Framework;
using namespace boost;

CAppConfig::CAppConfig() :
CConfig(CConfig::PathType(DEFAULT_CONFIG_PATH))
{

}

CAppConfig::~CAppConfig()
{

}
