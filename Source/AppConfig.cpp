#include "AppConfig.h"
#include "PathUtils.h"
#include <iostream>
#include <filesystem> // For file system operations

#define CONFIG_FILENAME ("config.xml")
#define PORTABLE_BASE_DATA_PATH ("Play Data Files")
#define BASE_DATA_PATH ("Play Data Files")

CAppConfig::CAppConfig()
    : CConfig(BuildConfigPath())
{
}

Framework::CConfig::PathType CAppConfig::GetBasePath()
{
	// Check for the presence of the "portable.txt" file
	if(std::filesystem::exists("portable.txt"))
	{
		//delete "read content portable.txt"
		
		// If "portable.txt" is present, simply use the path specified in PORTABLE_BASE_DATA_PATH
		auto basePath = PORTABLE_BASE_DATA_PATH;

		// Create the "Play Data Files" directory if it doesn't already exist
		std::filesystem::create_directories(basePath);

		return basePath; // Return the base path without the complete path
	}
	else
	{
		// If "portable.txt" is absent, use the original code for the base directory path
		auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;

		// Create the "Play Data Files" directory if it doesn't already exist
		Framework::PathUtils::EnsurePathExists(result);

		return result;
	}
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	auto basePath(GetBasePath());
	// "config.xml" is located in the directory of the base path
	return basePath / CONFIG_FILENAME;
}
