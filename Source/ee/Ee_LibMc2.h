#pragma once

#include "Types.h"
#include "MIPS.h"
#include "iop/IopBios.h"

class CPS2OS;
namespace Framework
{
	class CZipArchiveReader;
	class CZipArchiveWriter;
}

namespace Ee
{
	class CLibMc2
	{
	public:
		struct DIRPARAM
		{
			struct TIME
			{
				uint8 unknown;
				uint8 second;
				uint8 minute;
				uint8 hour;
				uint8 day;
				uint8 month;
				uint16 year;
			};

			TIME creationDate;
			TIME modificationDate;
			uint32 size;
			uint16 attributes;
			uint16 resv1;
			char name[32];
		};
		static_assert(sizeof(DIRPARAM) == 0x38);

		enum
		{
			SYSCALL_RANGE_START = 0x800,
			SYSCALL_MC2_CHECKASYNC = 0x800,
			SYSCALL_MC2_GETINFO_ASYNC = 0x802,
			SYSCALL_MC2_READFILE_ASYNC = 0x805,
			SYSCALL_MC2_WRITEFILE_ASYNC = 0x806,
			SYSCALL_MC2_CREATEFILE_ASYNC = 0x807,
			SYSCALL_MC2_DELETE_ASYNC = 0x808,
			SYSCALL_MC2_GETDIR_ASYNC = 0x80A,
			SYSCALL_MC2_MKDIR_ASYNC = 0x80B,
			SYSCALL_MC2_CHDIR_ASYNC = 0x80C,
			SYSCALL_MC2_CHMOD_ASYNC = 0x80D,
			SYSCALL_MC2_SEARCHFILE_ASYNC = 0x80E,
			SYSCALL_MC2_READFILE2_ASYNC = 0x820,
			SYSCALL_MC2_WRITEFILE2_ASYNC = 0x821,
			SYSCALL_RANGE_END,
		};

		CLibMc2(uint8*, CPS2OS&, CIopBios&);

		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);

		void HandleSyscall(CMIPS&);
		void NotifyVBlankStart();
		void HookLibMc2Functions();

	private:
		enum
		{
			WAIT_THREAD_ID_EMPTY = 0,
		};

		struct CARDINFO
		{
			uint32 type;
			uint32 formatted;
			uint32 freeClusters;
		};
		static_assert(sizeof(CARDINFO) == 0x0C);

		void OnIopModuleLoaded(const char*);
		uint32 AnalyzeFunction(uint32, int16);
		void WriteSyscall(uint32, uint16);

		void CheckAsync(CMIPS&);
		int32 GetInfoAsync(uint32, uint32);
		int32 CreateFileAsync(uint32, uint32);
		int32 DeleteAsync(uint32, uint32);
		int32 GetDirAsync(uint32, uint32, uint32, int32, uint32, uint32);
		int32 MkDirAsync(uint32, uint32);
		int32 ChDirAsync(uint32, uint32, uint32);
		int32 ChModAsync(uint32, uint32, uint32);
		int32 SearchFileAsync(uint32, uint32, uint32);
		int32 ReadFileAsync(uint32, uint32, uint32, uint32, uint32);
		int32 WriteFileAsync(uint32, uint32, uint32, uint32, uint32);

		static const char* GetSysCallDescription(uint16);

		uint8* m_ram = nullptr;
		CPS2OS& m_eeBios;
		CIopBios& m_iopBios;
		CIopBios::ModuleLoadedEvent::Connection m_moduleLoadedConnection;
		uint32 m_lastCmd = 0;
		uint32 m_lastResult = 0;
		uint32 m_waitThreadId = WAIT_THREAD_ID_EMPTY;
		uint32 m_waitVBlankCount = 0;

		uint32 m_getInfoAsyncPtr = 0;
		uint32 m_writeFileAsyncPtr = 0;
		uint32 m_createFileAsyncPtr = 0;
		uint32 m_deleteAsyncPtr = 0;
		uint32 m_getDirAsyncPtr = 0;
		uint32 m_mkDirAsyncPtr = 0;
		uint32 m_chDirAsyncPtr = 0;
		uint32 m_chModAsyncPtr = 0;
		uint32 m_searchFileAsyncPtr = 0;
		uint32 m_readFileAsyncPtr = 0;
		uint32 m_readFile2AsyncPtr = 0;
		uint32 m_writeFile2AsyncPtr = 0;
		uint32 m_checkAsyncPtr = 0;
	};
}
