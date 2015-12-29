#include "Iop_FileIoHandler2300.h"
#include "Iop_Ioman.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;

#define COMMANDID_OPEN        0
#define COMMANDID_CLOSE       1
#define COMMANDID_READ        2
#define COMMANDID_SEEK        4
#define COMMANDID_ACTIVATE    23

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
	case COMMANDID_ACTIVATE:
		*ret = InvokeActivate(args, argsSize, ret, retSize, ram);
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

	SendSifReply();
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

uint32 CFileIoHandler2300::InvokeActivate(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//No idea what that does...
	assert(retSize == 4);
	auto command = reinterpret_cast<ACTIVATECOMMAND*>(args);

	if(m_resultPtr[0] != 0)
	{
		//Send response
		ACTIVATEREPLY reply;
		reply.header.commandId = COMMANDID_ACTIVATE;
		CopyHeader(reply.header, command->header);
		reply.result   = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(ACTIVATEREPLY));
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
