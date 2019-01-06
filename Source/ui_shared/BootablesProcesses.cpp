#include <algorithm>
#include "BootablesProcesses.h"
#include "BootablesDbClient.h"
#include "LocalGamesDbClient.h"
#include "TheGamesDbClient.h"
#include "DiskUtils.h"
#include "string_format.h"
#include "../DiskUtils.h"

//Jobs
// Scan for new games (from input directory)
// Remove games that might not be available anymore
// Extract game ids from disk images
// Pull disc cover URLs and titles from GamesDb/TheGamesDb

bool IsBootableExecutablePath(const boost::filesystem::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".elf");
}

bool IsBootableDiscImagePath(const boost::filesystem::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".iso") ||
	       (extension == ".isz") ||
	       (extension == ".cso") ||
	       (extension == ".bin");
}

void ScanBootables(const boost::filesystem::path& parentPath, bool recursive)
{
	for(auto pathIterator = boost::filesystem::directory_iterator(parentPath);
	    pathIterator != boost::filesystem::directory_iterator(); pathIterator++)
	{
		auto& path = pathIterator->path();
		try
		{
			if(recursive && boost::filesystem::is_directory(path))
			{
				ScanBootables(path, recursive);
				continue;
			}
			std::string serial;
			if(
					!IsBootableExecutablePath(path) &&
					!(IsBootableDiscImagePath(path) && DiskUtils::TryGetDiskId(path, &serial)))
			{
				continue;
			}
			BootablesDb::CClient::GetInstance().RegisterBootable(path);
			BootablesDb::CClient::GetInstance().SetTitle(path, path.filename().string().c_str());
		}
		catch(const std::exception& exception)
		{
			//Failed to process a path, keep going
		}
	}
}

std::set<boost::filesystem::path> GetActiveBootableDirectories()
{
	std::set<boost::filesystem::path> result;
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		auto parentPath = bootable.path.parent_path();
		result.insert(parentPath);
	}
	return result;
}

void PurgeInexistingFiles()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		if(boost::filesystem::exists(bootable.path)) continue;
		BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
	}
}

void ExtractDiscIds()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		std::string discId;
		if(!DiskUtils::TryGetDiskId(bootable.path, &discId)) continue;
		BootablesDb::CClient::GetInstance().SetDiscId(bootable.path, discId.c_str());
	}
}

void FetchGameTitles()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	std::vector<std::string> serials;
	for(const auto& bootable : bootables)
	{
		if(bootable.discId.empty())
			continue;

		if(bootable.coverUrl.empty() || bootable.title.empty() || bootable.overview.empty())
			serials.push_back(bootable.discId);
	}

	if(serials.empty())
		return;

	auto gamesList = TheGamesDb::CClient::GetInstance().GetGames(serials);
	for(auto &game : gamesList)
	{
		for(const auto &bootable : bootables)
		{
			for(const auto &discId : game.discIds)
			{
				if(discId == bootable.discId)
				{
					BootablesDb::CClient::GetInstance().SetTitle(bootable.path, game.title.c_str());

					if(!game.overview.empty())
					{
						BootablesDb::CClient::GetInstance().SetOverview(bootable.path, game.overview.c_str());
					}
					if(!game.boxArtUrl.empty())
					{
						auto coverUrl = string_format("%s/%s", game.baseImgUrl.c_str(), game.boxArtUrl.c_str());
						BootablesDb::CClient::GetInstance().SetCoverUrl(bootable.path, coverUrl.c_str());
					}

					break;
				}
			}
		}
	}
}

uint32 GetTheGamesDbId(const char* discId)
{
	//Get the TheGamesDb ID from the local games db
	auto localGame = LocalGamesDb::CClient::GetInstance().GetGame(discId);
	if(localGame.theGamesDbId != 0)
	{
		return localGame.theGamesDbId;
	}

	//If no ID found in database, then, try a fuzzy lookup using the game name specified in the
	//local database
	auto gamesList = TheGamesDb::CClient::GetInstance().GetGamesList("11", localGame.title);
	if(gamesList.empty())
	{
		throw std::runtime_error("Game not found.");
	}
	else
	{
		//This is the one (might be wrong due to fuzzy search)
		auto gamesListItem = gamesList[0];
		return gamesListItem.id;
	}
}

std::string GetCoverUrl(const char* discId)
{
	try
	{
		auto theGamesDbId = GetTheGamesDbId(discId);
		auto theGamesDbGame = TheGamesDb::CClient::GetInstance().GetGame(theGamesDbId);
		auto coverUrl = string_format("%s/%s", theGamesDbGame.baseImgUrl.c_str(), theGamesDbGame.boxArtUrl.c_str());
		return coverUrl;
	}
	catch(const std::exception& exception)
	{
		return "";
	}
}

void FetchCoverUrls()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		//Don't fetch if bootable already has a coverUrl
		if(!bootable.coverUrl.empty()) continue;

		if(bootable.discId.empty()) continue;
		auto coverUrl = GetCoverUrl(bootable.discId.c_str());
		BootablesDb::CClient::GetInstance().SetCoverUrl(bootable.path, coverUrl.c_str());
	}
}
