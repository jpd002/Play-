#include "PsfVm.h"
#include "PsfLoader.h"
#include "MemoryUtils.h"
#include "CoffObjectFile.h"
#include "StdStreamUtils.h"
#include "Jitter.h"
#include "Jitter_CodeGenFactory.h"
#include "Jitter_CodeGen_x86_32.h"
#include "MemStream.h"
#include "Iop_PsfSubSystem.h"
#include "ThreadPool.h"
#include <boost/filesystem.hpp>
#include "Playlist.h"

namespace filesystem = boost::filesystem;

struct FUNCTION_TABLE_ITEM
{
	AOT_BLOCK_KEY	key;
	uint32			symbolIndex;
};
typedef std::vector<FUNCTION_TABLE_ITEM> FunctionTable;

typedef std::map<AOT_BLOCK_KEY, std::vector<uint32>> AotBlockMap;

extern "C" uint32 LWL_Proxy(uint32, uint32, CMIPS*);
extern "C" uint32 LWR_Proxy(uint32, uint32, CMIPS*);
extern "C" void SWL_Proxy(uint32, uint32, CMIPS*);
extern "C" void SWR_Proxy(uint32, uint32, CMIPS*);

void Gather(const char* archivePathName, const char* outputPathName)
{
	Framework::CStdStream outputStream(outputPathName, "wb");
	CBasicBlock::SetAotBlockOutputStream(&outputStream);

	{
		Framework::CThreadPool threadPool(std::thread::hardware_concurrency());

		filesystem::path archivePath = filesystem::path(archivePathName);
		auto archive = std::unique_ptr<CPsfArchive>(CPsfArchive::CreateFromPath(archivePath));

		for(auto fileInfoIterator = archive->GetFilesBegin();
			fileInfoIterator != archive->GetFilesEnd(); fileInfoIterator++)
		{
			filesystem::path archiveItemPath = fileInfoIterator->name;
			filesystem::path archiveItemExtension = archiveItemPath.extension();
			if(CPlaylist::IsLoadableExtension(archiveItemExtension.string().c_str() + 1))
			{
				threadPool.Enqueue(
					[=] ()
					{
						printf("Processing %s...\r\n", archiveItemPath.string().c_str());

						CPsfVm virtualMachine;

						CPsfLoader::LoadPsf(virtualMachine, archiveItemPath, archivePath);
						int currentTime = 0;
						virtualMachine.OnNewFrame.connect(
							[&currentTime] ()
							{
								currentTime += 16;
							});

						virtualMachine.Resume();

#ifdef _DEBUG
						static const unsigned int executionTime = 1;
#else
						static const unsigned int executionTime = 10;
#endif
						while(currentTime <= (executionTime * 60 * 1000))
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
						}

						virtualMachine.Pause();
					}
				);
			}
		}
	}

	CBasicBlock::SetAotBlockOutputStream(nullptr);
}

unsigned int CompileFunction(CPsfVm& virtualMachine, const std::vector<uint32>& blockCode, Jitter::CObjectFile& objectFile, const std::string& functionName, uint32 begin, uint32 end)
{
	auto& context = virtualMachine.GetCpu();

	uint8* ram = virtualMachine.GetRam();
	for(uint32 address = begin; address <= end; address += 4)
	{
		*reinterpret_cast<uint32*>(&ram[address]) = blockCode[(address - begin) / 4];
	}

	{
		Framework::CMemStream outputStream;
		Jitter::CObjectFile::INTERNAL_SYMBOL func;

		static CMipsJitter* jitter = nullptr;
		if(jitter == NULL)
		{
			Jitter::CCodeGen* codeGen = new Jitter::CCodeGen_x86_32();
			codeGen->SetExternalSymbolReferencedHandler(
				[&] (void* symbol, uint32 offset)
				{
					Jitter::CObjectFile::SYMBOL_REFERENCE ref;
					ref.offset		= offset;
					ref.type		= Jitter::CObjectFile::SYMBOL_TYPE_EXTERNAL;
					ref.symbolIndex	= objectFile.GetExternalSymbolIndexByValue(symbol);
					func.symbolReferences.push_back(ref);
				}
			);

			jitter = new CMipsJitter(codeGen);

			for(unsigned int i = 0; i < 4; i++)
			{
				jitter->SetVariableAsConstant(
					offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
					0
					);
			}
		}

		jitter->SetStream(&outputStream);
		jitter->Begin();

		for(uint32 address = begin; address <= end; address += 4)
		{
			context.m_pArch->CompileInstruction(address, jitter, &context);
			//Sanity check
			assert(jitter->IsStackEmpty());
		}

		jitter->End();

		func.name		= functionName;
		func.data		= std::vector<uint8>(outputStream.GetBuffer(), outputStream.GetBuffer() + outputStream.GetSize());
		func.location	= Jitter::CObjectFile::INTERNAL_SYMBOL_LOCATION_TEXT;
		return objectFile.AddInternalSymbol(func);
	}
}

AotBlockMap GetBlocksFromCache(const filesystem::path& blockCachePath)
{
	AotBlockMap result;

	auto path_end = filesystem::directory_iterator();
	for(auto pathIterator = filesystem::directory_iterator(blockCachePath); 
		pathIterator != path_end; pathIterator++)
	{
		const auto& filePath = (*pathIterator);
		printf("Processing %s...\r\n", filePath.path().string().c_str());

		auto blockCacheStream = Framework::CreateInputStdStream(filePath.path().native());

		uint32 fileSize = blockCacheStream.GetLength();
		while(fileSize != 0)
		{
			AOT_BLOCK_KEY key = {};
			key.crc		= blockCacheStream.Read32();
			key.begin	= blockCacheStream.Read32();
			key.end		= blockCacheStream.Read32();

			if(key.begin > key.end)
			{
				assert(0);
				throw std::runtime_error("Consistency error in block ranges.");
			}

			uint32 blockSize = (key.end - key.begin) + 4;

			std::vector<uint32> blockCode(blockSize / 4);
			blockCacheStream.Read(blockCode.data(), blockSize);

			auto blockIterator = result.find(key);
			if(blockIterator == std::end(result))
			{
				result.insert(std::make_pair(key, std::move(blockCode)));
			}
			else
			{
				if(!std::equal(std::begin(blockCode), std::end(blockCode), std::begin(blockIterator->second)))
				{
					assert(0);
					throw std::runtime_error("Block with same key already exists but with different data.");
				}
			}

			fileSize -= blockSize + 0x0C;
		}
	}

	return result;
}

void Compile(const char* databasePathName, const char* outputPath)
{
	CPsfVm virtualMachine;
	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(false);
	virtualMachine.SetSubSystem(subSystem);

	auto objectFile = std::unique_ptr<Jitter::CObjectFile>(new Jitter::CCoffObjectFile());
	objectFile->AddExternalSymbol("_MemoryUtils_GetByteProxy", &MemoryUtils_GetByteProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_GetHalfProxy", &MemoryUtils_GetHalfProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_GetWordProxy", &MemoryUtils_GetWordProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetByteProxy", &MemoryUtils_SetByteProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetHalfProxy", &MemoryUtils_SetHalfProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetWordProxy", &MemoryUtils_SetWordProxy);
	objectFile->AddExternalSymbol("_LWL_Proxy", &LWL_Proxy);
	objectFile->AddExternalSymbol("_LWR_Proxy", &LWR_Proxy);
	objectFile->AddExternalSymbol("_SWL_Proxy", &SWL_Proxy);
	objectFile->AddExternalSymbol("_SWR_Proxy", &SWR_Proxy);

	filesystem::path databasePath(databasePathName);
	auto blocks = GetBlocksFromCache(databasePath);

	printf("Got %d blocks to compile.\r\n", blocks.size());

	FunctionTable functionTable;
	functionTable.reserve(blocks.size());

	for(const auto& blockCachePair : blocks)
	{
		const auto& blockKey = blockCachePair.first;

		auto functionName = "aotblock_" + std::to_string(blockKey.crc) + "_" + std::to_string(blockKey.begin) + "_" + std::to_string(blockKey.end);

		unsigned int functionSymbolIndex = CompileFunction(virtualMachine, blockCachePair.second, *objectFile, functionName, blockKey.begin, blockKey.end);

		FUNCTION_TABLE_ITEM tableItem = { blockKey, functionSymbolIndex };
		functionTable.push_back(tableItem);
	}

	std::sort(functionTable.begin(), functionTable.end(), 
		[] (const FUNCTION_TABLE_ITEM& item1, const FUNCTION_TABLE_ITEM& item2)
		{
			return item1.key < item2.key;
		}
	);

	{
		Framework::CMemStream blockTableStream;
		Jitter::CObjectFile::INTERNAL_SYMBOL blockTableSymbol;
		blockTableSymbol.name		= "__aot_firstBlock";
		blockTableSymbol.location	= Jitter::CObjectFile::INTERNAL_SYMBOL_LOCATION_DATA;

		for(const auto& functionTableItem : functionTable)
		{
			blockTableStream.Write32(functionTableItem.key.crc);
			blockTableStream.Write32(functionTableItem.key.begin);
			blockTableStream.Write32(functionTableItem.key.end);
			
			{
				Jitter::CObjectFile::SYMBOL_REFERENCE ref;
				ref.offset		= static_cast<uint32>(blockTableStream.Tell());
				ref.type		= Jitter::CObjectFile::SYMBOL_TYPE_INTERNAL;
				ref.symbolIndex	= functionTableItem.symbolIndex;
				blockTableSymbol.symbolReferences.push_back(ref);
			}

			blockTableStream.Write32(0);
		}

		blockTableSymbol.data = std::vector<uint8>(blockTableStream.GetBuffer(), blockTableStream.GetBuffer() + blockTableStream.GetLength());
		objectFile->AddInternalSymbol(blockTableSymbol);
	}

	{
		Jitter::CObjectFile::INTERNAL_SYMBOL blockCountSymbol;
		blockCountSymbol.name		= "__aot_blockCount";
		blockCountSymbol.location	= Jitter::CObjectFile::INTERNAL_SYMBOL_LOCATION_DATA;
		blockCountSymbol.data		= std::vector<uint8>(4);
		*reinterpret_cast<uint32*>(blockCountSymbol.data.data()) = functionTable.size();
		objectFile->AddInternalSymbol(blockCountSymbol);
	}

	objectFile->Write(Framework::CStdStream(outputPath, "wb"));
}

void PrintUsage()
{
	printf("PsfAot usage:\r\n");
	printf("\tPsfAot gather [InputFile] [DatabasePath]\r\n");
	printf("\tPsfAot compile [DatabasePath] [x86|x64|arm] [coff|macho] [OutputFile]\r\n");
}

int main(int argc, char** argv)
{
	if(argc <= 2)
	{
		PrintUsage();
		return -1;
	}

	if(!strcmp(argv[1], "gather"))
	{
		if(argc < 4)
		{
			PrintUsage();
			return -1;
		}
		else
		{
			Gather(argv[2], argv[3]);
		}
	}
	else if(!strcmp(argv[1], "compile"))
	{
		if(argc < 6)
		{
			PrintUsage();
			return -1;
		}

		try
		{
			Compile(argv[2], argv[5]);
		}
		catch(const std::exception& exception)
		{
			printf("Failed to compile: %s\r\n", exception.what());
			return -1;
		}
	}

	return 0;
}
