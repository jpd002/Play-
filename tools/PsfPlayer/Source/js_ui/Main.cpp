#include "PsfVm.h"
#include <cstdio>
#include "filesystem_def.h"
#include "PsfLoader.h"
#include "PsfArchive.h"
#include "Jitter_CodeGen_Wasm.h"
#include "BasicBlock.h"
#include "MemoryUtils.h"
#include "SH_FileOutput.h"

CPsfVm* g_virtualMachine = nullptr;

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
		g_virtualMachine->SetSpuHandlerImpl(&CSH_FileOutput::HandlerFactory);
	}
	catch(const std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
	}
}

extern "C" void loadPsf()
{
	try
	{
		fs::path archivePath("work/Final Fantasy VII (EMU).zophar.zip");
		auto archive = std::unique_ptr<CPsfArchive>(CPsfArchive::CreateFromPath(archivePath));
		auto files = archive->GetFiles();
		auto fileToken = CArchivePsfStreamProvider::GetPathTokenFromFilePath(files.begin()->name);
		CPsfLoader::LoadPsf(*g_virtualMachine, fileToken, archivePath);
		printf("Loaded archive from '%s'\r\n", archivePath.string().c_str());
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
