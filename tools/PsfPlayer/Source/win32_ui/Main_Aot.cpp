#include "PsfVm.h"
#include "PsfLoader.h"
#include "MemoryUtils.h"
#include "CoffObjectFile.h"
#include "StdStreamUtils.h"
#include "Jitter.h"
#include "Jitter_CodeGenFactory.h"
#include "MemStream.h"
#include "Iop_PsfSubSystem.h"
#include <boost/filesystem.hpp>

namespace filesystem = boost::filesystem;

void Gather(const char* inputPath, const char* databasePathName)
{
	CBasicBlock::SetAotBlockOutputPath(databasePathName);

	CPsfVm virtualMachine;

	filesystem::path loadPath(inputPath);
	CPsfLoader::LoadPsf(virtualMachine, loadPath, filesystem::path());
	int currentTime = 0;
	virtualMachine.OnNewFrame.connect(
		[&currentTime] ()
		{
			currentTime += 16;
		});

	virtualMachine.Resume();

	while(currentTime <= (10 * 60 * 1000))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	virtualMachine.Pause();
}

unsigned int CompileFunction(CPsfVm& virtualMachine, Framework::CStream& inputStream, Jitter::CObjectFile& objectFile, const std::string& functionName, uint32 begin, uint32 end)
{
	auto& context = virtualMachine.GetCpu();

	uint8* ram = virtualMachine.GetRam();
	for(uint32 address = begin; address <= end; address += 4)
	{
		*reinterpret_cast<uint32*>(&ram[address]) = inputStream.Read32();
	}

	{
		Framework::CMemStream outputStream;
		Jitter::CObjectFile::INTERNAL_SYMBOL func;

		static CMipsJitter* jitter = nullptr;
		if(jitter == NULL)
		{
			Jitter::CCodeGen* codeGen = Jitter::CreateCodeGen();
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

struct FUNCTION_TABLE_ITEM
{
	uint32			crc;
	uint32			begin;
	uint32			end;
	uint32			symbolIndex;
};
typedef std::vector<FUNCTION_TABLE_ITEM> FunctionTable;

void Compile(const char* databasePathName, const char* outputPath)
{
	CPsfVm virtualMachine;
	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(false);
	virtualMachine.SetSubSystem(subSystem);

	auto objectFile = std::unique_ptr<Jitter::CObjectFile>(new Jitter::CCoffObjectFile());
	objectFile->AddExternalSymbol("_MemoryUtils_GetByteProxy", &CMemoryUtils::GetByteProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_GetHalfProxy", &CMemoryUtils::GetHalfProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_GetWordProxy", &CMemoryUtils::GetWordProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetByteProxy", &CMemoryUtils::SetByteProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetHalfProxy", &CMemoryUtils::SetHalfProxy);
	objectFile->AddExternalSymbol("_MemoryUtils_SetWordProxy", &CMemoryUtils::SetWordProxy);

	FunctionTable functionTable;
	
	filesystem::path databasePath(databasePathName);
	auto path_end = filesystem::directory_iterator();

	auto fileCount = std::distance(filesystem::directory_iterator(databasePath), path_end);
	functionTable.reserve(fileCount);

	for(auto pathIterator = filesystem::directory_iterator(databasePath); 
		pathIterator != path_end; pathIterator++)
	{
		const auto& filePath = (*pathIterator);
		printf("Compiling %s...\r\n", filePath.path().string().c_str());

		auto codeStream = Framework::CreateInputStdStream(filePath.path().native());
		uint64 fileLength = codeStream.GetLength();
		assert((fileLength & 0x03) == 0);

		uint32 crc = codeStream.Read32();
		uint32 begin = codeStream.Read32();
		uint32 end = codeStream.Read32();
		assert(fileLength == ((end - begin) + 0x04 + 0x0C));

		auto functionName = filePath.path().stem().string();

		unsigned int functionSymbolIndex = CompileFunction(virtualMachine, codeStream, *objectFile, functionName, begin, end);

		FUNCTION_TABLE_ITEM tableItem = { crc, begin, end, functionSymbolIndex };
		functionTable.push_back(tableItem);
	}

	std::sort(functionTable.begin(), functionTable.end(), 
		[] (const FUNCTION_TABLE_ITEM& item1, const FUNCTION_TABLE_ITEM& item2)
		{
			if(item1.crc == item2.crc)
			{
				if(item1.begin == item2.begin)
				{
					return item1.end < item2.end;
				}
				else
				{
					return item1.begin < item2.begin;
				}
			}
			else
			{
				return item1.crc < item2.crc;
			}
		}
	);

	{
		Framework::CMemStream blockTableStream;
		Jitter::CObjectFile::INTERNAL_SYMBOL blockTableSymbol;
		blockTableSymbol.name		= "__aot_firstBlock";
		blockTableSymbol.location	= Jitter::CObjectFile::INTERNAL_SYMBOL_LOCATION_DATA;

		for(const auto& functionTableItem : functionTable)
		{
			blockTableStream.Write32(functionTableItem.crc);
			blockTableStream.Write32(functionTableItem.begin);
			blockTableStream.Write32(functionTableItem.end);
			
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
