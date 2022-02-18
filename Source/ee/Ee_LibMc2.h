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
			SYSCALL_MC2_GETENTSPACE_ASYNC = 0x80F,
			SYSCALL_MC2_READFILE2_ASYNC = 0x820,
			SYSCALL_MC2_WRITEFILE2_ASYNC = 0x821,
			SYSCALL_MC2_GETDBCSTATUS = 0x900,
			SYSCALL_RANGE_END,
		};

		CLibMc2(uint8*, CPS2OS&, CIopBios&);

		void Reset();

		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);

		void HandleSyscall(CMIPS&);
		void NotifyVBlankStart();
		void HookLibMc2Functions();

	private:
		enum
		{
			WAIT_THREAD_ID_EMPTY = 0,
			WAIT_VBLANK_INIT_COUNT = 4,
		};

		struct MODULE_FUNCTIONS
		{
			uint32 getInfoAsyncPtr = 0;
			uint32 readFileAsyncPtr = 0;
			uint32 writeFileAsyncPtr = 0;
			uint32 createFileAsyncPtr = 0;
			uint32 deleteAsyncPtr = 0;
			uint32 getDirAsyncPtr = 0;
			uint32 mkDirAsyncPtr = 0;
			uint32 chDirAsyncPtr = 0;
			uint32 chModAsyncPtr = 0;
			uint32 searchFileAsyncPtr = 0;
			uint32 getEntSpaceAsyncPtr = 0;
			uint32 readFile2AsyncPtr = 0;
			uint32 writeFile2AsyncPtr = 0;
			uint32 checkAsyncPtr = 0;
			uint32 getDbcStatusPtr = 0;
		};

		struct CARDINFO
		{
			uint32 type;
			uint32 formatted;
			uint32 freeClusters;
		};
		static_assert(sizeof(CARDINFO) == 0x0C);

		void OnIopModuleLoaded(const char*);
		uint32 AnalyzeFunction(MODULE_FUNCTIONS&, uint32, int16);
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
		int32 GetEntSpaceAsync(uint32, uint32);
		int32 ReadFileAsync(uint32, uint32, uint32, uint32, uint32);
		int32 WriteFileAsync(uint32, uint32, uint32, uint32, uint32);
		int32 GetDbcStatus(uint32, uint32);

		static const char* GetSysCallDescription(uint16);

		uint8* m_ram = nullptr;
		CPS2OS& m_eeBios;
		CIopBios& m_iopBios;
		CIopBios::ModuleLoadedEvent::Connection m_moduleLoadedConnection;
		uint32 m_lastCmd = 0;
		uint32 m_lastResult = 0;
		uint32 m_waitThreadId = WAIT_THREAD_ID_EMPTY;
		uint32 m_waitVBlankCount = 0;
	};
}
