#include "BootablesProcesses.h"
#include "BootablesDbClient.h"
#include "LocalGamesDbClient.h"
#include "TheGamesDbClient.h"
#include "DiskUtils.h"
#include "string_format.h"

//Jobs
// Scan for new games (from input directory)
// Remove games that might not be available anymore
// Extract game ids from disk images
// Pull disc cover URLs and titles from GamesDb/TheGamesDb

void ScanBootables(const boost::filesystem::path& parentPath)
{
	for(auto pathIterator = boost::filesystem::directory_iterator(parentPath);
		pathIterator != boost::filesystem::directory_iterator(); pathIterator++)
	{
		auto& path = pathIterator->path();
		if(boost::filesystem::is_directory(path))
		{
			ScanBootables(path);
			continue;
		}
		auto pathExtension = path.extension();
		if(
			(pathExtension != ".isz") &&
			(pathExtension != ".elf")
		) continue;
		BootablesDb::CClient::GetInstance().RegisterBootable(path);
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
	for(const auto& bootable : bootables)
	{
		if(bootable.discId.empty()) continue;
		try
		{
			auto game = LocalGamesDb::CClient::GetInstance().GetGame(bootable.discId.c_str());
			BootablesDb::CClient::GetInstance().SetTitle(bootable.path, game.title.c_str());
		}
		catch(...)
		{
			//Log or something?
		}
	}
}

std::string GetCoverUrl(const char* discId)
{
	uint32 theGamesDbId = 0;

	try
	{
		auto localGame = LocalGamesDb::CClient::GetInstance().GetGame(discId);
		theGamesDbId = localGame.theGamesDbId;
		if(theGamesDbId == 0)
		{
			//If no ID found in database, then, try a fuzzy lookup using the game name specified in the
			//legacy database
			auto gamesList = TheGamesDb::CClient::GetInstance().GetGamesList("sony playstation 2", localGame.title);
			if(gamesList.empty())
			{
				//Nothing found, we're screwed
			}
			else
			{
				//This is the one (might be wrong due to fuzzy search)
				auto gamesListItem = gamesList[0];
				theGamesDbId = gamesListItem.id;
			}
		}
	}
	catch(const std::exception& exception)
	{
		return "";
	}

	try
	{
		if(theGamesDbId != 0)
		{
			//We've got an ID, so, we can fetch information for that one
			auto theGamesDbGame = TheGamesDb::CClient::GetInstance().GetGame(theGamesDbId);
			auto imageUrl = string_format("%s/%s", theGamesDbGame.baseImgUrl.c_str(), theGamesDbGame.boxArtUrl.c_str());
			return imageUrl;
		}
	}
	catch(const std::exception& exception)
	{
		printf("Failed to obtain game info: '%s'.\r\n", exception.what());
		return "";
	}
}

void FetchCoverUrls()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		if(bootable.discId.empty()) continue;
		auto coverUrl = GetCoverUrl(bootable.discId.c_str());
		BootablesDb::CClient::GetInstance().SetCoverUrl(bootable.path, coverUrl.c_str());
	}
}
