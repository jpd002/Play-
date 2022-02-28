#pragma once

#include <cassert>
#include <array>
#include "Iop_FileIo.h"
#include "Ioman_Defs.h"

namespace Iop
{
	class CFileIoHandler2200 : public CFileIo::CHandler
	{
	public:
		CFileIoHandler2200(CIoman*, CSifMan&);

		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		void ProcessCommands(CSifMan*) override;

	private:
		struct PENDINGREPLY
		{
			enum
			{
				REPLY_BUFFER_SIZE = 0x20,
			};

			template <typename T>
			void SetReply(const T& reply)
			{
				assert(!valid);
				static_assert(sizeof(T) <= REPLY_BUFFER_SIZE, "Reply buffer too small.");
				valid = true;
				replySize = sizeof(T);
				memcpy(buffer.data(), &reply, sizeof(T));
			}

			bool valid = false;
			uint32 fileId = 0;
			uint32 replySize = 0;
			std::array<uint8, REPLY_BUFFER_SIZE> buffer;
		};

		enum COMMANDID
		{
			COMMANDID_OPEN = 0,
			COMMANDID_CLOSE = 1,
			COMMANDID_READ = 2,
			COMMANDID_WRITE = 3,
			COMMANDID_SEEK = 4,
			COMMANDID_REMOVE = 6,
			COMMANDID_MKDIR = 7,
			COMMANDID_RMDIR = 8,
			COMMANDID_DOPEN = 9,
			COMMANDID_DCLOSE = 10,
			COMMANDID_DREAD = 11,
			COMMANDID_GETSTAT = 12,
			COMMANDID_CHSTAT = 13,
			COMMANDID_FORMAT = 14,
			COMMANDID_CHDIR = 18,
			COMMANDID_SYNC = 19,
			COMMANDID_MOUNT = 20,
			COMMANDID_UMOUNT = 21,
			COMMANDID_SEEK64 = 22,
			COMMANDID_DEVCTL = 23,
			COMMANDID_IOCTL2 = 26,
		};

		struct COMMANDHEADER
		{
			uint32 semaphoreId;
			uint32 resultPtr;
			uint32 resultSize;
		};

		struct OPENCOMMAND
		{
			COMMANDHEADER header;
			uint32 flags;
			uint32 somePtr1;
			char fileName[256];
		};

		struct CLOSECOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
		};

		struct READCOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			uint32 buffer;
			uint32 size;
		};

		struct WRITECOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			uint32 buffer;
			uint32 size;
			uint32 unalignedSize;
			uint8 unalignedData[64];
		};

		struct SEEKCOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			int32 offset;
			uint32 whence;
		};

		struct MKDIRCOMMAND
		{
			COMMANDHEADER header;
			uint32 mode;
			char dirName[256];
		};

		struct DOPENCOMMAND
		{
			COMMANDHEADER header;
			char dirName[256];
		};

		struct DCLOSECOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
		};

		struct DREADCOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			uint32 dirEntryPtr;
		};

		struct GETSTATCOMMAND
		{
			COMMANDHEADER header;
			uint32 statBuffer;
			char fileName[256];
		};

		struct CHSTATCOMMAND
		{
			COMMANDHEADER header;
			uint32 flags;
			Ioman::STATEX stat;
			char path[256];
		};

		struct FORMATCOMMAND
		{
			COMMANDHEADER header;
			char device[0x400];
			char blockDevice[0x400];
			char args[0x400];
			uint32 argsSize;
		};

		struct CCODECOMMAND
		{
			COMMANDHEADER header;
			char path[256];
		};

		struct SYNCCOMMAND
		{
			COMMANDHEADER header;
			char deviceName[0x400];
			uint32 flags;
		};

		struct MOUNTCOMMAND
		{
			COMMANDHEADER header;
			char fileSystemName[0x100];
			char unused[0x300];
			char deviceName[0x400];
		};

		struct UMOUNTCOMMAND
		{
			COMMANDHEADER header;
			char deviceName[0x100];
		};

		struct SEEK64COMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			int64 offset;
			uint32 whence;
			uint32 dummy;
		};
		static_assert(sizeof(SEEK64COMMAND) == 0x20, "SEEK64COMMAND needs to be 0x20 bytes long.");

		struct DEVCTLCOMMAND
		{
			COMMANDHEADER header;
			char device[0x100];
			char unused[0x300];
			char inputBuffer[0x400];
			uint32 cmdId;
			uint32 inputSize;
			uint32 outputPtr;
			uint32 outputSize;
		};

		struct IOCTL2COMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			uint32 cmdId;
			char inputBuffer[0x400];
			uint32 outputPtr;
			uint32 outputSize;
			uint32 inputSize;
		};

		struct REPLYHEADER
		{
			uint32 semaphoreId;
			uint32 commandId;
			uint32 resultPtr;
			uint32 resultSize;
		};

		struct GENERICREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2 = 0;
			uint32 unknown3 = 0;
			uint32 unknown4 = 0;
		};

		struct CLOSEREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct READREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct DREADREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 dirEntryPtr;
			Ioman::STATEX stat;
			char name[Ioman::DIRENTRY::NAME_SIZE];
		};
		static_assert(sizeof(DREADREPLY) == 0x158, "DREADREPLY must be 0x158 bytes long.");

		struct GETSTATREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 dstPtr;
			Ioman::STAT stat;
		};

		struct SEEK64REPLY
		{
			REPLYHEADER header;
			uint64 result;
			uint32 unknown3;
			uint32 unknown4;
		};

		uint32 InvokeOpen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeClose(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeRead(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeWrite(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeSeek(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeRemove(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeMkdir(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeRmdir(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDopen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDclose(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDread(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeGetStat(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeChstat(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeFormat(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeChdir(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeSync(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeMount(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeUmount(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeSeek64(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDevctl(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeIoctl2(uint32*, uint32, uint32*, uint32, uint8*);

		void CopyHeader(REPLYHEADER&, const COMMANDHEADER&);
		void PrepareGenericReply(uint8*, const COMMANDHEADER&, COMMANDID, uint32);
		void SendPendingReply(uint8*);
		void SendSifReply();

		CSifMan& m_sifMan;
		uint32 m_resultPtr[2];
		PENDINGREPLY m_pendingReply;
	};
};
