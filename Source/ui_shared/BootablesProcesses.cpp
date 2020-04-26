#include <algorithm>
#include "AppConfig.h"
#include "BootablesProcesses.h"
#include "BootablesDbClient.h"
#include "TheGamesDbClient.h"
#include "DiskUtils.h"
#include "PathUtils.h"
#include "string_format.h"
#include "StdStreamUtils.h"
#include "http/HttpClientFactory.h"

//Jobs
// Scan for new games (from input directory)
// Remove games that might not be available anymore
// Extract game ids from disk images
// Pull disc cover URLs and titles from GamesDb/TheGamesDb

static void BootableLog(const char* format, ...)
{
	static FILE* logStream = nullptr;
	if(!logStream)
	{
		auto logPath = CAppConfig::GetBasePath() / "bootables.log";
		logStream = fopen(logPath.string().c_str(), "wb");
	}
	va_list args;
	va_start(args, format);
	vfprintf(logStream, format, args);
	va_end(args);
	fflush(logStream);
}

bool IsBootableExecutablePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".elf");
}

bool IsBootableDiscImagePath(const fs::path& filePath)
{
	auto extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return (extension == ".iso") ||
	       (extension == ".isz") ||
	       (extension == ".cso") ||
	       (extension == ".bin");
}

bool TryRegisteringBootable(const fs::path& path)
{
	std::string serial;
	if(
	    !BootablesDb::CClient::GetInstance().BootableExist(path) &&
	    !IsBootableExecutablePath(path) &&
	    !(IsBootableDiscImagePath(path) && DiskUtils::TryGetDiskId(path, &serial)))
	{
		return false;
	}
	BootablesDb::CClient::GetInstance().RegisterBootable(path, path.filename().string().c_str(), serial.c_str());
	return true;
}

void ScanBootables(const fs::path& parentPath, bool recursive)
{
	BootableLog("Entering ScanBootables(path = '%s', recursive = %d);\r\n",
		parentPath.string().c_str(), static_cast<int>(recursive));
	try
	{
		for(auto pathIterator = fs::directory_iterator(parentPath);
			pathIterator != fs::directory_iterator(); pathIterator++)
		{
			auto& path = pathIterator->path();
			BootableLog("Checking '%s'... ", path.string().c_str());
			try
			{
				if(recursive && fs::is_directory(path))
				{
					BootableLog("is directory.\r\n");
					ScanBootables(path, recursive);
					continue;
				}
				BootableLog("registering... ");
				bool success = TryRegisteringBootable(path);
				BootableLog("result = %d\r\n", static_cast<int>(success));
			}
			catch(const std::exception& exception)
			{
				//Failed to process a path, keep going
				BootableLog(" exception: %s\r\n", exception.what());
			}
		}
	}
	catch(const std::exception& exception)
	{
		BootableLog("Caught an exception while trying to list directory: %s\r\n", exception.what());
	}
	BootableLog("Exiting ScanBootables(path = '%s', recursive = %d);\r\n",
		parentPath.string().c_str(), static_cast<int>(recursive));
}

std::set<fs::path> GetActiveBootableDirectories()
{
	std::set<fs::path> result;
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		auto parentPath = bootable.path.parent_path();
		static const char* s3ImagePathPrefix = "//s3/";
		if(parentPath.string().find(s3ImagePathPrefix) == std::string::npos)
			result.insert(parentPath);
	}
	return result;
}

void PurgeInexistingFiles()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(const auto& bootable : bootables)
	{
		if(fs::exists(bootable.path)) continue;
		BootablesDb::CClient::GetInstance().UnregisterBootable(bootable.path);
	}
}

void FetchGameTitles()
{
	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	std::vector<std::string> serials;
	for(const auto& bootable : bootables)
	{
		if(bootable.discId.empty()) continue;

		if(bootable.coverUrl.empty() || bootable.title.empty() || bootable.overview.empty())
		{
			serials.push_back(bootable.discId);
		}
	}

	if(serials.empty()) return;

	try
	{
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
							auto coverUrl = string_format("%s%s", game.baseImgUrl.c_str(), game.boxArtUrl.c_str());
							BootablesDb::CClient::GetInstance().SetCoverUrl(bootable.path, coverUrl.c_str());
						}

						break;
					}
				}
			}
		}
	}
	catch(...)
	{
	}
}

void FetchGameCovers()
{
	auto coverpath(CAppConfig::GetBasePath() / fs::path("covers"));
	Framework::PathUtils::EnsurePathExists(coverpath);

	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	std::vector<std::string> serials;
	for(const auto& bootable : bootables)
	{
		if(bootable.discId.empty() || bootable.coverUrl.empty())
			continue;

		auto path = coverpath / (bootable.discId + ".jpg");
		if(fs::exists(path))
			continue;

		auto requestResult =
		    [&]() {
			    auto client = Framework::Http::CreateHttpClient();
			    client->SetUrl(bootable.coverUrl);
			    return client->SendRequest();
		    }();
		if(requestResult.statusCode == Framework::Http::HTTP_STATUS_CODE::OK)
		{
			auto outputStream = Framework::CreateOutputStdStream(path.native());
			outputStream.Write(requestResult.data.GetBuffer(), requestResult.data.GetSize());
		}
	}
}
