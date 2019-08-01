#pragma once

#include "Iop_FileIo.h"
#include "Iop_Ioman.h"

namespace Iop
{
	class CFileIoHandler2240 : public CFileIo::CHandler
	{
	public:
		CFileIoHandler2240(CIoman*, CSifMan&);

		void Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		void ProcessCommands() override;

	private:
		enum COMMANDID
		{
			COMMANDID_OPEN = 0,
			COMMANDID_CLOSE = 1,
			COMMANDID_READ = 2,
			COMMANDID_SEEK = 4,
			COMMANDID_DOPEN = 9,
			COMMANDID_GETSTAT = 12,
			COMMANDID_MOUNT = 20,
			COMMANDID_UMOUNT = 21,
			COMMANDID_DEVCTL = 23,
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

		struct SEEKCOMMAND
		{
			COMMANDHEADER header;
			uint32 fd;
			uint32 offset;
			uint32 whence;
		};

		struct DOPENCOMMAND
		{
			COMMANDHEADER header;
			char dirName[256];
		};

		struct GETSTATCOMMAND
		{
			COMMANDHEADER header;
			uint32 statBuffer;
			char fileName[256];
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

		struct REPLYHEADER
		{
			uint32 semaphoreId;
			uint32 commandId;
			uint32 resultPtr;
			uint32 resultSize;
		};

		struct OPENREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
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

		struct SEEKREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct DOPENREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct GETSTATREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 dstPtr;
			CIoman::STAT stat;
		};

		struct MOUNTREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct UMOUNTREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		struct DEVCTLREPLY
		{
			REPLYHEADER header;
			uint32 result;
			uint32 unknown2;
			uint32 unknown3;
			uint32 unknown4;
		};

		uint32 InvokeOpen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeClose(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeRead(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeSeek(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDopen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeGetStat(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeMount(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeUmount(uint32*, uint32, uint32*, uint32, uint8*);
		uint32 InvokeDevctl(uint32*, uint32, uint32*, uint32, uint8*);

		void CopyHeader(REPLYHEADER&, const COMMANDHEADER&);
		void SendSifReply();

		uint32 m_resultPtr[2];
		CSifMan& m_sifMan;
		bool m_pendingReadCommand = false;
	};
};
