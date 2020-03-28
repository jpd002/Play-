#pragma once

#include "Types.h"
#include "MIPS.h"

class CIopBios;

namespace Ee
{
	class CLibMc2
	{
	public:
		enum
		{
			SYSCALL_RANGE_START = 0x800,
			SYSCALL_MC2_CHECKASYNC = 0x800,
			SYSCALL_MC2_GETINFO_ASYNC = 0x802,
			SYSCALL_MC2_WRITEFILE_ASYNC = 0x806,
			SYSCALL_MC2_CREATEFILE_ASYNC = 0x807,
			SYSCALL_MC2_GETDIR_ASYNC = 0x80A,
			SYSCALL_MC2_MKDIR_ASYNC = 0x80B,
			SYSCALL_MC2_SEARCHFILE_ASYNC = 0x80E,
			SYSCALL_MC2_READFILE_ASYNC = 0x820,
			SYSCALL_RANGE_END,
		};

		CLibMc2(uint8*, CIopBios&);
		void HandleSyscall(CMIPS&);
		void HookLibMc2Functions();

	private:
		struct CARDINFO
		{
			uint32 type;
			uint32 formatted;
			uint32 freeClusters;
		};
		static_assert(sizeof(CARDINFO) == 0x0C);

		struct DIRPARAM
		{
			uint32 creationDate0;
			uint32 creationDate1;
			uint32 modifDate0;
			uint32 modifDate1;
			uint32 size;
			uint16 attributes;
			uint16 resv1;
			char name[32];
		};
		static_assert(sizeof(DIRPARAM) == 0x38);

		uint32 AnalyzeFunction(uint32, int16);
		void WriteSyscall(uint32, uint16);

		int32 CheckAsync(uint32, uint32, uint32);
		int32 GetInfoAsync(uint32, uint32);
		int32 CreateFileAsync(uint32, uint32);
		int32 GetDirAsync(uint32, uint32, uint32, int32, uint32, uint32);
		int32 MkDirAsync(uint32, uint32);
		int32 SearchFileAsync(uint32, uint32, uint32);
		int32 ReadFileAsync(uint32, uint32, uint32, uint32, uint32);
		int32 WriteFileAsync(uint32, uint32, uint32, uint32, uint32);

		uint8* m_ram = nullptr;
		CIopBios& m_iopBios;
		uint32 m_lastCmd = 0;
		uint32 m_lastResult = 0;

		uint32 m_getInfoAsyncPtr = 0;
		uint32 m_writeFileAsyncPtr = 0;
		uint32 m_createFileAsyncPtr = 0;
		uint32 m_getDirAsyncPtr = 0;
		uint32 m_mkDirAsyncPtr = 0;
		uint32 m_searchFileAsyncPtr = 0;
		uint32 m_readFileAsyncPtr = 0;
		uint32 m_checkAsyncPtr = 0;
	};
}
