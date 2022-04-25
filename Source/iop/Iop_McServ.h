#pragma once

#include <string>
#include <map>
#include <regex>
#include "filesystem_def.h"
#include "StdStream.h"
#include "Iop_Module.h"
#include "Iop_SifMan.h"

class CMIPSAssembler;
class CIopBios;

namespace Iop
{
	class CSysmem;
	class CSifCmd;

	class CMcServ : public CModule, public CSifModule
	{
	public:
		enum CMD_ID
		{
			CMD_ID_NONE = 0x00,
			CMD_ID_GETINFO = 0x01,
			CMD_ID_OPEN = 0x02,
			CMD_ID_CLOSE = 0x03,
			CMD_ID_SEEK = 0x04,
			CMD_ID_READ = 0x05,
			CMD_ID_WRITE = 0x06,
			CMD_ID_CHDIR = 0x0C,
			CMD_ID_GETDIR = 0x0D,
			CMD_ID_SETFILEINFO = 0x0E,
			CMD_ID_DELETE = 0x0F,
			CMD_ID_GETENTSPACE = 0x12,
			CMD_ID_SETTHREADPRIORITY = 0x14,
		};

		enum OPEN_FLAGS
		{
			OPEN_FLAG_RDONLY = 0x00000001,
			OPEN_FLAG_WRONLY = 0x00000002,
			OPEN_FLAG_RDWR = 0x00000003,
			OPEN_FLAG_CREAT = 0x00000200,
			OPEN_FLAG_TRUNC = 0x00000400,
		};

		enum FILE_ATTRIBUTE_FLAGS
		{
			MC_FILE_ATTR_READABLE = 0x01,
			MC_FILE_ATTR_WRITEABLE = 0x02,
			MC_FILE_ATTR_EXECUTABLE = 0x04,
			MC_FILE_ATTR_DUP_PROHIBIT = 0x08,
			MC_FILE_ATTR_FILE = 0x10,
			MC_FILE_ATTR_SUBDIR = 0x20,
			MC_FILE_CREATE_DIR = 0x0040,
			MC_FILE_ATTR_CLOSED = 0x0080,
			MC_FILE_CREATE_FILE = 0x0200,
			MC_FILE_0400 = 0x0400,
			MC_FILE_ATTR_PDAEXEC = 0x0800,
			MC_FILE_ATTR_PS1 = 0x1000,
			MC_FILE_ATTR_HIDDEN = 0x2000,
			MC_FILE_ATTR_EXISTS = 0x8000,
		};

		enum RETURN_CODES
		{
			RET_OK = 0,
			RET_NO_ENTRY = -4,
			RET_PERMISSION_DENIED = -5,
			RET_NOT_EMPTY = -6,
		};

		struct CMD
		{
			uint32 port;
			uint32 slot;
			uint32 flags;
			int32 maxEntries;
			uint32 tableAddress;
			char name[0x400];
		};
		static_assert(sizeof(CMD) == 0x414, "Size of CMD structure must be 0x414 bytes.");

		struct FILECMD
		{
			uint32 handle;
			uint32 pad[2];
			uint32 size;
			uint32 offset;
			uint32 origin;
			uint32 bufferAddress;
			uint32 paramAddress;
			char data[16];
		};

		struct ENTRY
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

			TIME creationTime;
			TIME modificationTime;
			uint32 size;
			uint16 attributes;
			uint16 reserved0;
			uint32 reserved1[2];
			uint8 name[0x20];
		};

		CMcServ(CIopBios&, CSifMan&, CSifCmd&, CSysmem&, uint8*);
		virtual ~CMcServ() = default;

		static const char* GetMcPathPreference(unsigned int);
		static std::string EncodeMcName(const std::string&);
		static std::string DecodeMcName(const std::string&);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		void CountTicks(uint32, CSifMan*);

	private:
		struct MODULEDATA
		{
			enum
			{
				TRAMPOLINE_SIZE = 0x80,
				RPC_BUFFER_SIZE = 0x80,
			};

			SIFRPCCLIENTDATA rpcClientData;
			uint8 rpcBuffer[RPC_BUFFER_SIZE];
			uint32 initialized = 0;
			uint32 readFastHandle = 0;
			uint32 readFastSize = 0;
			uint32 readFastBufferAddress = 0;
			uint32 pendingCommand = 0;
			uint32 pendingCommandDelay = 0;
			uint8 trampoline[TRAMPOLINE_SIZE];
		};

		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
		};

		enum
		{
			MAX_FILES = 5,
			MAX_PORTS = 2,
			MAX_SLOTS = 1,
		};

		class CPathFinder
		{
		public:
			CPathFinder();
			virtual ~CPathFinder();

			void Reset();
			void Search(const fs::path&, const char*);
			unsigned int Read(ENTRY*, unsigned int);

		private:
			typedef std::vector<ENTRY> EntryList;

			void SearchRecurse(const fs::path&);
			uint32 CountEntries(const fs::path&);

			EntryList m_entries;
			fs::path m_basePath;
			std::regex m_filterExp;
			unsigned int m_index;
		};

		void BuildCustomCode();
		uint32 AssembleReadFast(CMIPSAssembler&);

		void GetInfo(uint32*, uint32, uint32*, uint32, uint8*);
		void Open(uint32*, uint32, uint32*, uint32, uint8*);
		void Close(uint32*, uint32, uint32*, uint32, uint8*);
		void Seek(uint32*, uint32, uint32*, uint32, uint8*);
		void Read(uint32*, uint32, uint32*, uint32, uint8*);
		void Write(uint32*, uint32, uint32*, uint32, uint8*);
		void Flush(uint32*, uint32, uint32*, uint32, uint8*);
		void ChDir(uint32*, uint32, uint32*, uint32, uint8*);
		void GetDir(uint32*, uint32, uint32*, uint32, uint8*);
		void Delete(uint32*, uint32, uint32*, uint32, uint8*);
		void SetFileInfo(uint32*, uint32, uint32*, uint32, uint8*);
		void GetEntSpace(uint32*, uint32, uint32*, uint32, uint8*);
		void SetThreadPriority(uint32*, uint32, uint32*, uint32, uint8*);
		void GetSlotMax(uint32*, uint32, uint32*, uint32, uint8*);
		bool ReadFast(uint32*, uint32, uint32*, uint32, uint8*);
		void WriteFast(uint32*, uint32, uint32*, uint32, uint8*);
		void GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);

		bool HandleInvalidPortOrSlot(uint32, uint32, uint32*);

		void StartReadFast(CMIPS&);
		void ProceedReadFast(CMIPS&);
		void FinishReadFast(CMIPS&);

		uint32 GenerateHandle();
		Framework::CStdStream* GetFileFromHandle(uint32);
		fs::path GetAbsoluteFilePath(unsigned int, unsigned int, const char*) const;

		CIopBios& m_bios;
		CSifMan& m_sifMan;
		CSifCmd& m_sifCmd;
		CSysmem& m_sysMem;
		uint8* m_ram = nullptr;
		uint32 m_moduleDataAddr = 0;
		uint32 m_startReadFastAddr = 0;
		uint32 m_proceedReadFastAddr = 0;
		uint32 m_finishReadFastAddr = 0;
		uint32 m_readFastAddr = 0;
		Framework::CStdStream m_files[MAX_FILES];
		static const char* m_mcPathPreference[MAX_PORTS];
		std::string m_currentDirectory[MAX_PORTS];
		CPathFinder m_pathFinder;

		// Keeps track, if the memory card in
		// a given slot has already been read,
		// or if it is a newly inserted card.
		bool m_knownMemoryCards[MAX_PORTS];
	};

	typedef std::shared_ptr<CMcServ> McServPtr;
}
