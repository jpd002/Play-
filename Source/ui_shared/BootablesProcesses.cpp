#include "BootablesProcesses.h"
#include "BootablesDbClient.h"
#include "DiskUtils.h"

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

}
