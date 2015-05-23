#include <stdio.h>
#include <assert.h>
#include "iop/Iop_McServ.h"
#include "iop/Iop_SifManNull.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"
#include "GameTestSheet.h"

void PrepareTestEnvironment(const CGameTestSheet::EnvironmentActionArray& environment)
{
	//TODO: There's a bug if there's a slash at the end of the path for the memory card
	auto mcPathPreference = Iop::CMcServ::GetMcPathPreference(0);
	auto memoryCardPath = boost::filesystem::path("./memorycard");
	boost::filesystem::remove_all(memoryCardPath);

	CAppConfig::GetInstance().RegisterPreferenceString(mcPathPreference, "");
	CAppConfig::GetInstance().SetPreferenceString(mcPathPreference, memoryCardPath.string().c_str());
	
	for(const auto& environmentAction : environment)
	{
		if(environmentAction.type == CGameTestSheet::ENVIRONMENT_ACTION_CREATE_DIRECTORY)
		{
			auto folderToCreate = memoryCardPath / boost::filesystem::path(environmentAction.name);
			Framework::PathUtils::EnsurePathExists(folderToCreate);
		}
		else if(environmentAction.type == CGameTestSheet::ENVIRONMENT_ACTION_CREATE_FILE)
		{
			auto fileToCreate = memoryCardPath / boost::filesystem::path(environmentAction.name);
			auto inputStream = Framework::CreateOutputStdStream(fileToCreate.native());
			inputStream.Seek(environmentAction.size - 1, Framework::STREAM_SEEK_SET);
			inputStream.Write8(0x00);
		}
	}
}

void ExecuteTest(const CGameTestSheet::TEST& test)
{
	Iop::CSifManNull sifMan;
	Iop::CMcServ mcServ(sifMan);

	if(!test.currentDirectory.empty())
	{
		uint32 result = 0;

		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		assert(test.currentDirectory.size() <= sizeof(cmd.name));
		strncpy(cmd.name, test.currentDirectory.c_str(), sizeof(cmd.name));

		mcServ.Invoke(0xC, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), nullptr);
	}

	{
		uint32 result = 0;

		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.maxEntries = test.maxEntries;
		assert(test.query.length() <= sizeof(cmd.name));
		strncpy(cmd.name, test.query.c_str(), sizeof(cmd.name));

		std::vector<Iop::CMcServ::ENTRY> entries;
		if(cmd.maxEntries > 0)
		{
			entries.resize(cmd.maxEntries);
		}
		mcServ.Invoke(0xD, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

		assert(result == test.result);
		for(unsigned int i = 0; i < test.entries.size(); i++)
		{
			const auto& entry = entries[i];
			const auto& refEntry = test.entries[i];
			if(strcmp(reinterpret_cast<const char*>(entry.name), refEntry.c_str()))
			{
				assert(0);
			}
		}
	}
}

int main(int argc, const char** argv)
{
	auto testsPath = boost::filesystem::path("./tests/");

	boost::filesystem::directory_iterator endDirectoryIterator;
	for(boost::filesystem::directory_iterator directoryIterator(testsPath);
		directoryIterator != endDirectoryIterator; directoryIterator++)
	{
		auto testPath = directoryIterator->path();
		CGameTestSheet testSheet(Framework::CreateInputStdStream(testPath.native()));

		for(const auto& test : testSheet.GetTests())
		{
			auto environment = testSheet.GetEnvironment(test.environmentId);
			PrepareTestEnvironment(environment);
			ExecuteTest(test);
		}
	}

	return 0;
}
