#include "PsfVm.h"
#include <cstdio>
#include "filesystem_def.h"
#include "PsfLoader.h"
#include "PsfArchive.h"
#include "Jitter_CodeGen_Wasm.h"
#include "BasicBlock.h"
#include "MemoryUtils.h"
#include "SH_FileOutput.h"
#include "../SH_OpenAL.h"
#include <emscripten/bind.h>

CPsfVm* g_virtualMachine = nullptr;
CPsfBase::TagMap g_tags;

int main(int argc, const char** argv)
{
	return 0;
}

extern "C" void initVm()
{
	try
	{
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&EmptyBlockHandler), "_EmptyBlockHandler", "vi");
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_GetByteProxy), "_MemoryUtils_GetByteProxy", "iii");
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_GetHalfProxy), "_MemoryUtils_GetHalfProxy", "iii");
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_GetWordProxy), "_MemoryUtils_GetWordProxy", "iii");

		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_SetByteProxy), "_MemoryUtils_SetByteProxy", "viii");
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_SetHalfProxy), "_MemoryUtils_SetHalfProxy", "viii");
		Jitter::CWasmFunctionRegistry::RegisterFunction(reinterpret_cast<uintptr_t>(&MemoryUtils_SetWordProxy), "_MemoryUtils_SetWordProxy", "viii");

		g_virtualMachine = new CPsfVm();
		//g_virtualMachine->SetSpuHandlerImpl(&CSH_FileOutput::HandlerFactory);
		g_virtualMachine->SetSpuHandlerImpl(&CSH_OpenAL::HandlerFactory);
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
}

extern "C" void loadPsf(const char* archivePath, const char* psfPath)
{
	printf("Loading '%s' from '%s'.\r\n", psfPath, archivePath);
	try
	{
		assert(g_virtualMachine);
		g_tags.clear();
		auto fileToken = CArchivePsfStreamProvider::GetPathTokenFromFilePath(psfPath);
		CPsfLoader::LoadPsf(*g_virtualMachine, fileToken, archivePath, &g_tags);
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
}

extern "C" void step()
{
	try
	{
		g_virtualMachine->StepSync();
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

	function("getPsfArchiveFileList", &getPsfArchiveFileList);
	function("getCurrentPsfTags", &getCurrentPsfTags);
}
