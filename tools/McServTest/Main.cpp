#include <stdio.h>
#include <assert.h>
#include "iop/IopBios.h"
#include "iop/Iop_McServ.h"
#include "iop/Iop_PathUtils.h"
#include "iop/Iop_SubSystem.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"
#include "GameTestSheet.h"

#define CHECK(condition)        \
	if(!(condition))            \
	{                           \
		throw std::exception(); \
	}

void PrepareTestEnvironment(const CGameTestSheet::EnvironmentActionArray& environment)
{
	//TODO: There's a bug if there's a slash at the end of the path for the memory card
	auto mcPathPreference = Iop::CMcServ::GetMcPathPreference(0);
	auto memoryCardPath = fs::path("./memorycard");
	fs::remove_all(memoryCardPath);

	CAppConfig::GetInstance().RegisterPreferencePath(mcPathPreference, "");
	CAppConfig::GetInstance().SetPreferencePath(mcPathPreference, memoryCardPath);

	for(const auto& environmentAction : environment)
	{
		auto actionName = Iop::CMcServ::EncodeMcName(environmentAction.name);
		if(environmentAction.type == CGameTestSheet::ENVIRONMENT_ACTION_CREATE_DIRECTORY)
		{
			auto folderToCreate = Iop::PathUtils::MakeHostPath(memoryCardPath, actionName.c_str());
			Framework::PathUtils::EnsurePathExists(folderToCreate);
		}
		else if(environmentAction.type == CGameTestSheet::ENVIRONMENT_ACTION_CREATE_FILE)
		{
			auto fileToCreate = Iop::PathUtils::MakeHostPath(memoryCardPath, actionName.c_str());
			auto inputStream = Framework::CreateOutputStdStream(fileToCreate.native());
			inputStream.Seek(environmentAction.size - 1, Framework::STREAM_SEEK_SET);
			inputStream.Write8(0x00);
		}
	}
}

void ExecuteTest(const CGameTestSheet::TEST& test)
{
	Iop::CSubSystem subSystem(true);
	subSystem.Reset();
	auto bios = static_cast<CIopBios*>(subSystem.m_bios.get());
	bios->Reset(std::shared_ptr<Iop::CSifMan>());
	auto mcServ = bios->GetMcServ();

	if(!test.currentDirectory.empty())
	{
		uint32 result = 0;

		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		assert(test.currentDirectory.size() <= sizeof(cmd.name));
		strncpy(cmd.name, test.currentDirectory.c_str(), sizeof(cmd.name));

		mcServ->Invoke(Iop::CMcServ::CMD_ID_CHDIR, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), nullptr);

		CHECK(result == 0);
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
		mcServ->Invoke(Iop::CMcServ::CMD_ID_GETDIR, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

		CHECK(result == test.result);

		//Check that we can find all reference entries in the result
		//Items don't need to be in the same order, but need to appear only once
		for(const auto& refEntry : test.entries)
		{
			auto entryCount = std::count_if(entries.begin(), entries.end(),
			                                [&refEntry](const auto& entry) {
				                                return strcmp(reinterpret_cast<const char*>(entry.name), refEntry.c_str()) == 0;
			                                });
			CHECK(entryCount == 1);
		}
	}
}

int main(int argc, const char** argv)
{
	auto testsPath = fs::path("./tests/");

	fs::directory_iterator endDirectoryIterator;
	for(fs::directory_iterator directoryIterator(testsPath);
	    directoryIterator != endDirectoryIterator; directoryIterator++)
	{
		auto testPath = directoryIterator->path();
		auto stream = Framework::CreateInputStdStream(testPath.native());
		CGameTestSheet testSheet(stream);

		for(const auto& test : testSheet.GetTests())
		{
			auto environment = testSheet.GetEnvironment(test.environmentId);
			PrepareTestEnvironment(environment);
			ExecuteTest(test);
		}
	}

	return 0;
}
