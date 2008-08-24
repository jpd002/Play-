#include "AppConfig.h"
#include "PathUtils.h"

#define DEFAULT_CONFIG_PATH     ("config.xml")

using namespace Framework;
using namespace boost;

CAppConfig::CAppConfig() :
CConfig(BuildConfigPath())
{

}

CAppConfig::~CAppConfig()
{

}

CConfig::PathType CAppConfig::BuildConfigPath()
{
#if defined(MACOSX)
	passwd* userInfo = getpwuid(getuid());
	if(userInfo == NULL) return DEFAULT_CONFIG_PATH;
	return string(userInfo->pw_dir) + "/Library/Preferences/com.vapps.Purei.xml";
#elif defined(WIN32)
    CConfig::PathType userPath(PathUtils::GetRoamingDataPath() / L"Virtual Applications" / L"Purei");
    PathUtils::EnsurePathExists(userPath);
//    CConfig::PathType companyPath = userPath / L"Virtual Applications";
//    CConfig::PathType productPath = companyPath / L"Purei";
//    try
//    {
//        filesystem::create_directory(companyPath);
//        filesystem::create_directory(productPath);
//    }
//    catch(...)
//    {
//        //Creation failed (maybe because it already exists)
//    }
    return (userPath / L"Config.xml");
#else
	return DEFAULT_CONFIG_PATH;
#endif
}
