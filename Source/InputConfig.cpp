#include "AppConfig.h"
#include "InputConfig.h"
#include "PathUtils.h"

#define PROFILE_PATH ("inputprofiles")

CInputConfig::CInputConfig(const Framework::CConfig::PathType& path)
    : CConfig(path)
{
}

bool CInputConfig::IsValidProfileName(std::string name)
{
	static const std::string valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";
	for(auto c : name)
	{
		if(valid_chars.find(c) == std::string::npos)
		{
			return false;
		}
	}
	return true;
}

std::unique_ptr<CInputConfig> CInputConfig::LoadProfile(std::string name)
{
	auto path = GetProfilePath() / name;
	path.replace_extension(".xml");
	return std::make_unique<CInputConfig>(path);
}

Framework::CConfig::PathType CInputConfig::GetProfilePath()
{
	auto profile_path = CAppConfig::GetInstance().GetBasePath() / PROFILE_PATH;
	Framework::PathUtils::EnsurePathExists(profile_path);
	return profile_path;
}

Framework::CConfig::PathType CInputConfig::GetProfile(std::string name)
{
	auto profile_path = CAppConfig::GetInstance().GetBasePath() / PROFILE_PATH;
	Framework::PathUtils::EnsurePathExists(profile_path);
	profile_path /= name;
	profile_path.replace_extension(".xml");
	return profile_path;
}
