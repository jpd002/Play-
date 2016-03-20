#include "Iop_FileIoHandler2300.h"
#include "Iop_Ioman.h"
#include "../RegisterStateFile.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

#define STATE_XML            ("iop_fileio/state2300.xml")
#define STATE_RESULTPTR0     ("resultPtr0")
#define STATE_RESULTPTR1     ("resultPtr1")
#define STATE_PENDINGREADCMD ("pendingReadCmd")

using namespace Iop;

#define COMMANDID_OPEN        0
#define COMMANDID_CLOSE       1
#define COMMANDID_READ        2
#define COMMANDID_SEEK        4
#define COMMANDID_DOPEN       9
#define COMMANDID_GETSTAT     12
#define COMMANDID_DEVCTL      23

#define DEVCTL_CDVD_GETERROR   0x4320
#define DEVCTL_CDVD_DISKREADY  0x4325

CFileIoHandler2300::CFileIoHandler2300(CIoman* ioman, CSifMan& sifMan)
: CHandler(ioman)
, m_sifMan(sifMan)
{
	memset(m_resultPtr, 0, sizeof(m_resultPtr));
}

void CFileIoHandler2300::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case COMMANDID_OPEN:
		*ret = InvokeOpen(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_CLOSE:
		*ret = InvokeClose(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_READ:
		*ret = InvokeRead(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_SEEK:
		*ret = InvokeSeek(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DOPEN:
		*ret = InvokeDopen(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_GETSTAT:
		*ret = InvokeGetStat(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DEVCTL:
		*ret = InvokeDevctl(args, argsSize, ret, retSize, ram);
		break;
	case 255:
		//Not really sure about that...
		if(retSize == 8)
		{
			memcpy(ret, "....rawr", 8);
		}
		else if(retSize == 4)
		{
			memcpy(ret, "....", 4);
		}
		else
		{
			assert(0);
		}
		m_resultPtr[0] = args[0];
		m_resultPtr[1] = args[1];
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
}

void CFileIoHandler2300::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_XML));
	m_resultPtr[0]       = registerFile.GetRegister32(STATE_RESULTPTR0);
	m_resultPtr[1]       = registerFile.GetRegister32(STATE_RESULTPTR1);
	m_pendingReadCommand = registerFile.GetRegister32(STATE_PENDINGREADCMD) != 0;
}

void CFileIoHandler2300::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = new CRegisterStateFile(STATE_XML);
	registerFile->SetRegister32(STATE_RESULTPTR0,     m_resultPtr[0]);
	registerFile->SetRegister32(STATE_RESULTPTR1,     m_resultPtr[1]);
	registerFile->SetRegister32(STATE_PENDINGREADCMD, m_pendingReadCommand ? 1 : 0);
	archive.InsertFile(registerFile);
}

void CFileIoHandler2300::ProcessCommands()
{
	if(m_pendingReadCommand)
	{
		SendSifReply();
		m_pendingReadCommand = false;
	}
}

uint32 CFileIoHandler2300::InvokeOpen(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<OPENCOMMAND*>(args);
	auto result = m_ioman->Open(command->flags, command->fileName);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		OPENREPLY reply;
		reply.header.commandId = COMMANDID_OPEN;
		CopyHeader(reply.header, command->header);
		reply.result   = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(OPENREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2300::InvokeClose(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<CLOSECOMMAND*>(args);
	auto result = m_ioman->Close(command->fd);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		CLOSEREPLY reply;
		reply.header.commandId = COMMANDID_CLOSE;
		CopyHeader(reply.header, command->header);
		reply.result   = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(CLOSEREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2300::InvokeRead(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<READCOMMAND*>(args);
	auto result = m_ioman->Read(command->fd, command->size, reinterpret_cast<void*>(ram + command->buffer));

	//Send response
	if(m_resultPtr[0] != 0)
	{
		READREPLY reply;
		reply.header.commandId = COMMANDID_READ;
		CopyHeader(reply.header, command->header);
		reply.result   = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(READREPLY));
	}

	//Delay read reply to next frame (needed by Phantasy Star Collection)
	assert(!m_pendingReadCommand);
	m_pendingReadCommand = true;
	return 1;
}

uint32 CFileIoHandler2300::InvokeSeek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<SEEKCOMMAND*>(args);
	auto result = m_ioman->Seek(command->fd, command->offset, command->whence);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		SEEKREPLY reply;
		reply.header.commandId = COMMANDID_SEEK;
		CopyHeader(reply.header, command->header);
		reply.result   = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(SEEKREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2300::InvokeDopen(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<DOPENCOMMAND*>(args);

	//This is a stub and is not implemented yet
	CLog::GetInstance().Print(LOG_NAME, "Dopen('%s');\r\n", command->dirName);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		DOPENREPLY reply;
		reply.header.commandId = COMMANDID_DOPEN;
		CopyHeader(reply.header, command->header);
		reply.result   = -1;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(SEEKREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2300::InvokeGetStat(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	CIoman::STAT stat;
	auto command = reinterpret_cast<GETSTATCOMMAND*>(args);
	auto result = m_ioman->GetStat(command->fileName, &stat);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		GETSTATREPLY reply;
		reply.header.commandId = COMMANDID_GETSTAT;
		CopyHeader(reply.header, command->header);
		reply.result = result;
		reply.dstPtr = command->statBuffer;
		reply.stat   = stat;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(GETSTATREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2300::InvokeDevctl(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Romancing Saga with 'hdd0:' parameter.
	//This is also used by Phantasy Star Collection with 'cdrom0:' parameter.

	assert(argsSize >= 0x81C);
	assert(retSize == 4);
	auto command = reinterpret_cast<DEVCTLCOMMAND*>(args);

	uint32* input = reinterpret_cast<uint32*>(command->inputBuffer);
	uint32* output = reinterpret_cast<uint32*>(ram + command->outputPtr);

	switch(command->cmdId)
	{
	case DEVCTL_CDVD_GETERROR:
		assert(command->outputSize == 4);
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> CdGetError();\r\n");
		output[0] = 0;	//No error
		break;
	case DEVCTL_CDVD_DISKREADY:
		assert(command->inputSize == 4);
		assert(command->outputSize == 4);
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> CdDiskReady(%d);\r\n", input[0]);
		output[0] = 2;	//Disk ready
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> Unknown(cmd = %0.8X);\r\n", command->cmdId);
		break;
	}

	if(m_resultPtr[0] != 0)
	{
		//Send response
		DEVCTLREPLY reply;
		reply.header.commandId = COMMANDID_DEVCTL;
		CopyHeader(reply.header, command->header);
		reply.result   = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(DEVCTLREPLY));
	}

	SendSifReply();
	return 1;
}

void CFileIoHandler2300::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
	reply.semaphoreId = command.semaphoreId;
	reply.resultPtr = command.resultPtr;
	reply.resultSize = command.resultSize;
}

void CFileIoHandler2300::SendSifReply()
{
	size_t packetSize = sizeof(SIFCMDHEADER);
	uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
	auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
	header->commandId = 0x80000011;
	header->size = packetSize;
	header->dest = 0;
	header->optional = 0;
	m_sifMan.SendPacket(callbackPacket, packetSize);
}
