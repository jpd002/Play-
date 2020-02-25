#pragma once

#include "Config.h"

#define DEFAULT_PROFILE ("default")

class CInputConfig : public Framework::CConfig
{
public:
	CInputConfig(const Framework::CConfig::PathType& path);
	virtual ~CInputConfig() = default;

	static bool IsValidProfileName(std::string);
	static CConfig::PathType GetProfilePath();
	static CConfig::PathType GetProfile(std::string = DEFAULT_PROFILE);
	static std::unique_ptr<CInputConfig> LoadProfile(std::string = DEFAULT_PROFILE);
};
