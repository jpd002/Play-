#include "PsfVmJs.h"
#include <cstdio>
#include "filesystem_def.h"
#include "PsfArchive.h"
#include "SH_OpenALProxy.h"
#include "SH_FileOutput.h"
#include "../SH_OpenAL.h"
#include <emscripten/bind.h>

CPsfVmJs* g_virtualMachine = nullptr;
CSH_OpenAL* g_soundHandler = nullptr;
CPsfBase::TagMap g_tags;

int main(int argc, const char** argv)
{
	return 0;
}

extern "C" void initVm()
{
	try
	{
		g_virtualMachine = new CPsfVmJs();

		//g_soundHandler = new CSH_FileOutput();
		g_soundHandler = new CSH_OpenAL();
		g_virtualMachine->SetSpuHandler(CSH_OpenALProxy::GetFactoryFunction(g_soundHandler));
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
}

void resumePsf()
{
	assert(g_virtualMachine);
	g_virtualMachine->Resume();
}

void pausePsf()
{
	assert(g_virtualMachine);
	g_virtualMachine->Pause();
}

void loadPsf(std::string archivePath, std::string psfPath)
{
	printf("Loading '%s' from '%s'.\r\n", psfPath.c_str(), archivePath.c_str());
	try
	{
		assert(g_virtualMachine);
		g_virtualMachine->Pause();
		g_tags = g_virtualMachine->LoadPsf(archivePath, psfPath);
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
}

std::vector<std::string> getPsfArchiveFileList(std::string rawArchivePath)
{
	std::vector<std::string> result;
	try
	{
		fs::path archivePath(rawArchivePath);
		auto archive = std::unique_ptr<CPsfArchive>(CPsfArchive::CreateFromPath(archivePath));
		auto files = archive->GetFiles();
		for(const auto& file : files)
		{
			result.push_back(file.name);
		}
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
	return result;
}

CPsfBase::TagMap getCurrentPsfTags()
{
	return g_tags;
}

EMSCRIPTEN_BINDINGS(PsfPlayer)
{
	using namespace emscripten;

	register_vector<std::string>("StringList");
	register_map<std::string, std::string>("StringMap");

	function("pausePsf", &pausePsf);
	function("resumePsf", &resumePsf);
	function("loadPsf", &loadPsf);
	function("getPsfArchiveFileList", &getPsfArchiveFileList);
	function("getCurrentPsfTags", &getCurrentPsfTags);
}
