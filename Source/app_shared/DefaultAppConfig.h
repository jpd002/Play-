#pragma once

//Include this file in your app's main.cpp file to obtain a default AppConfig base path.

#include "PathUtils.h"
#include "AppConfig.h"

fs::path CAppConfig::GetBasePath() const
{
	static const char* BASE_DATA_PATH = "Play Data Files";
	static const auto basePath =
	    []() {
		    auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
		    Framework::PathUtils::EnsurePathExists(result);
		    return result;
	    }();
	return basePath;
}
