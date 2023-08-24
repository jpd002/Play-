#include "AppConfig.h"
#include "PathUtils.h"
#include <iostream>
#include <fstream>    // Pour la lecture du fichier texte
#include <filesystem> // Pour les opérations sur le système de fichiers

#define CONFIG_FILENAME ("config.xml")
#define PORTABLE_BASE_DATA_PATH ("Play Data Files")
#define BASE_DATA_PATH ("Play Data Files")

CAppConfig::CAppConfig()
    : CConfig(BuildConfigPath())
{
}

Framework::CConfig::PathType CAppConfig::GetBasePath()
{
	// Check the presence of the "portable.txt" file
	if(std::filesystem::exists("portable.txt"))
	{
		// Read the "portable.txt" text file to get the base path
		std::ifstream configFile("portable.txt");
		std::string basePath;

		if(configFile.is_open())
		{
			std::getline(configFile, basePath);
			configFile.close();
		}

		// If "portable.txt" is empty, use "Play Data Files" in the application folder
		if(basePath.empty())
		{
			// Just use the path specified in PORTABLE_BASE_DATA_PATH
			basePath = PORTABLE_BASE_DATA_PATH;
		}
		else
		{
			// If "portable.txt" contains a path, use this path
// relative or absolute path
			if(basePath[0] == '\\' || basePath[0] == '/')
			{
				// Remove the first character if it is a path separator
				basePath.erase(0, 1);
			}
		}

		// Create the "Play Data Files" directory if it does not already exist
		std::filesystem::create_directories(basePath);

		return basePath; // Retournez le chemin de base sans le chemin complet
	}
	else
	{
		// If "portable.txt" is missing, use original code for base directory path
		auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;

		// Create the "Play Data Files" directory if it does not already exist
		Framework::PathUtils::EnsurePathExists(result);

		return result;
	}
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	auto basePath(GetBasePath());
	// "config.xml" is located in the base path directory
	return basePath / CONFIG_FILENAME;
}
