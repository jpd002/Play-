#include <algorithm>
#include "BootablesProcesses.h"
#include "BootablesDbClient.h"
#include "TheGamesDbClient.h"
#include "DiskUtils.h"
#include "string_format.h"

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
			BootablesDb::CClient::GetInstance().RegisterBootable(path, path.filename().string().c_str(), serial.c_str());
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
	for(auto& game : gamesList)
	{
		for(const auto& bootable : bootables)
		{
			for(const auto& discId : game.discIds)
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
