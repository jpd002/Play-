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
	// Vérifiez la présence du fichier "portable.txt"
	if(std::filesystem::exists("portable.txt"))
	{
		// Lire le fichier texte "portable.txt" pour obtenir le chemin de base
		std::ifstream configFile("portable.txt");
		std::string basePath;

		if(configFile.is_open())
		{
			std::getline(configFile, basePath);
			configFile.close();
		}

		// Si "portable.txt" est vide, utilisez "Play Data Files" dans le dossier de l'application
		if(basePath.empty())
		{
			// Utilisez simplement le chemin spécifié dans PORTABLE_BASE_DATA_PATH
			basePath = PORTABLE_BASE_DATA_PATH;
		}
		else
		{
			// Si "portable.txt" contient un chemin, utilisez ce chemin
			// chemin relatif ou absolu
			if(basePath[0] == '\\' || basePath[0] == '/')
			{
				// Supprimez le premier caractère s'il est un séparateur de chemin
				basePath.erase(0, 1);
			}
		}

		// Créez le répertoire "Play Data Files" s'il n'existe pas déjà
		std::filesystem::create_directories(basePath);

		return basePath; // Retournez le chemin de base sans le chemin complet
	}
	else
	{
		// Si "portable.txt" est absent, utilisez le code d'origine pour le chemin du répertoire de base
		auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;

		// Créez le répertoire "Play Data Files" s'il n'existe pas déjà
		Framework::PathUtils::EnsurePathExists(result);

		return result;
	}
}

Framework::CConfig::PathType CAppConfig::BuildConfigPath()
{
	auto basePath(GetBasePath());
	// "config.xml" est situé dans le répertoire du chemin de base
	return basePath / CONFIG_FILENAME;
}
