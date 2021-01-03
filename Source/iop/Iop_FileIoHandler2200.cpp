#include <cassert>
#include <cstring>
#include "Iop_FileIoHandler2200.h"
#include "Iop_Ioman.h"
#include "Iop_SifManPs2.h"
#include "../states/RegisterStateFile.h"
#include "../states/MemoryStateFile.h"
#include "../Log.h"
#include "../Ps2Const.h"

#define LOG_NAME ("iop_fileio")

#define STATE_XML ("iop_fileio/state2200.xml")
#define STATE_RESULTPTR0 ("resultPtr0")
#define STATE_RESULTPTR1 ("resultPtr1")
#define STATE_PENDINGREPLY ("iop_fileio/state2200_pending")

using namespace Iop;

#define DEVCTL_CDVD_GETERROR 0x4320
#define DEVCTL_CDVD_DISKREADY 0x4325

#define DEVCTL_HDD_STATUS 0x4807
#define DEVCTL_HDD_FREESECTOR 0x480A

CFileIoHandler2200::CFileIoHandler2200(CIoman* ioman, CSifMan& sifMan)
    : CHandler(ioman)
    , m_sifMan(sifMan)
{
	memset(m_resultPtr, 0, sizeof(m_resultPtr));
	memset(m_pendingReply.buffer.data(), 0, PENDINGREPLY::REPLY_BUFFER_SIZE);
}

bool CFileIoHandler2200::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
	case COMMANDID_CCODE:
		*ret = InvokeCcode(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_MOUNT:
		*ret = InvokeMount(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_UMOUNT:
		*ret = InvokeUmount(args, argsSize, ret, retSize, ram);
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
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
	return true;
}

void CFileIoHandler2200::LoadState(Framework::CZipArchiveReader& archive)
{
	{
		auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_XML));
		m_resultPtr[0] = registerFile.GetRegister32(STATE_RESULTPTR0);
		m_resultPtr[1] = registerFile.GetRegister32(STATE_RESULTPTR1);
	}

	archive.BeginReadFile(STATE_PENDINGREPLY)->Read(&m_pendingReply, sizeof(m_pendingReply));
}

void CFileIoHandler2200::SaveState(Framework::CZipArchiveWriter& archive) const
{
	{
		auto registerFile = new CRegisterStateFile(STATE_XML);
		registerFile->SetRegister32(STATE_RESULTPTR0, m_resultPtr[0]);
		registerFile->SetRegister32(STATE_RESULTPTR1, m_resultPtr[1]);
		archive.InsertFile(registerFile);
	}

	{
		auto memoryFile = new CMemoryStateFile(STATE_PENDINGREPLY, &m_pendingReply, sizeof(m_pendingReply));
		archive.InsertFile(memoryFile);
	}
}

void CFileIoHandler2200::ProcessCommands(CSifMan* sifMan)
{
	if(m_pendingReply.valid)
	{
		uint8* eeRam = nullptr;
		if(auto sifManPs2 = dynamic_cast<CSifManPs2*>(sifMan))
		{
			eeRam = sifManPs2->GetEeRam();
		}
		SendPendingReply(eeRam);
	}
}

uint32 CFileIoHandler2200::InvokeOpen(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
		reply.result = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(OPENREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeClose(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<CLOSECOMMAND*>(args);
	auto fileMode = m_ioman->GetFileMode(command->fd);
	auto result = m_ioman->Close(command->fd);

	CLOSEREPLY reply;
	reply.header.commandId = COMMANDID_CLOSE;
	CopyHeader(reply.header, command->header);
	reply.result = result;
	reply.unknown2 = 0;
	reply.unknown3 = 0;
	reply.unknown4 = 0;

	//1945 1+2 will close the file before checking the read command has completed.
	//Just push the read command reply and queue a close command reply.
	//Assuming this is only possible if a file is opened with NOWAIT.
	if(m_pendingReply.valid && (m_pendingReply.fileId == command->fd))
	{
		assert((fileMode & Ioman::CDevice::OPEN_FLAG_NOWAIT) != 0);
		SendPendingReply(ram);
		assert(!m_pendingReply.valid);
		m_pendingReply.SetReply(reply);
		m_pendingReply.fileId = command->fd;
	}
	else
	{
		//Send response
		if(m_resultPtr[0] != 0)
		{
			memcpy(ram + m_resultPtr[0], &reply, sizeof(CLOSEREPLY));
		}

		SendSifReply();
	}

	return 1;
}

uint32 CFileIoHandler2200::InvokeRead(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<READCOMMAND*>(args);
	uint32 readAddress = command->buffer & (PS2::EE_RAM_SIZE - 1);
	auto result = m_ioman->Read(command->fd, command->size, reinterpret_cast<void*>(ram + readAddress));

	READREPLY reply;
	reply.header.commandId = COMMANDID_READ;
	CopyHeader(reply.header, command->header);
	reply.result = result;
	reply.unknown2 = 0;
	reply.unknown3 = 0;
	reply.unknown4 = 0;

	//Delay read reply to next frame.
	//Some games, like Shadow of the Colossus, seem to rely on the delay to
	//work properly (probably because it causes EE threads to be rescheduled).
	m_pendingReply.SetReply(reply);
	m_pendingReply.fileId = command->fd;
	return 1;
}

uint32 CFileIoHandler2200::InvokeSeek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
		reply.result = result;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(SEEKREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeDopen(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
		reply.result = -1;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(DOPENREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeGetStat(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	Ioman::STAT stat;
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
		reply.stat = stat;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(GETSTATREPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeCcode(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Gigawing Generations
	assert(argsSize >= 0xD);
	assert(retSize == 4);
	auto command = reinterpret_cast<CCODECOMMAND*>(args);

	CLog::GetInstance().Print(LOG_NAME, "CCode('%s');\r\n",
	                          command->path);

	if(m_resultPtr[0] != 0)
	{
		//Send response
		CCODEREPLY reply;
		reply.header.commandId = COMMANDID_CCODE;
		CopyHeader(reply.header, command->header);
		reply.result = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(CCODEREPLY));
	}

	SendSifReply();
	//Not supported for now, return 0
	return 0;
}

uint32 CFileIoHandler2200::InvokeMount(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Final Fantasy X
	assert(argsSize >= 0xC14);
	assert(retSize == 4);
	auto command = reinterpret_cast<MOUNTCOMMAND*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Mount('%s', '%s');\r\n",
	                          command->fileSystemName, command->deviceName);

	if(m_resultPtr[0] != 0)
	{
		//Send response
		MOUNTREPLY reply;
		reply.header.commandId = COMMANDID_MOUNT;
		CopyHeader(reply.header, command->header);
		reply.result = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(MOUNTREPLY));
	}

	SendSifReply();
	//Not supported for now, return 0
	return 0;
}

uint32 CFileIoHandler2200::InvokeUmount(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Romancing Saga with 'pfs0:' parameter.
	assert(argsSize >= 0x0C);
	assert(retSize == 4);
	auto command = reinterpret_cast<UMOUNTCOMMAND*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Umount('%s');\r\n", command->deviceName);

	if(m_resultPtr[0] != 0)
	{
		//Send response
		UMOUNTREPLY reply;
		reply.header.commandId = COMMANDID_UMOUNT;
		CopyHeader(reply.header, command->header);
		reply.result = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(UMOUNTREPLY));
	}

	SendSifReply();
	//Not supported for now, return 0
	return 0;
}

uint32 CFileIoHandler2200::InvokeDevctl(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Romancing Saga and FFX with 'hdd0:' parameter.
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
		output[0] = 0; //No error
		break;
	case DEVCTL_CDVD_DISKREADY:
		assert(command->inputSize == 4);
		assert(command->outputSize == 4);
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> CdDiskReady(%d);\r\n", input[0]);
		output[0] = 2; //Disk ready
		break;
	case DEVCTL_HDD_STATUS:
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> HddStatus();\r\n");
		break;
	case DEVCTL_HDD_FREESECTOR:
		assert(command->outputSize == 4);
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> HddFreeSector();\r\n");
		output[0] = 0x400000; //Number of sectors
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "DevCtl -> Unknown(cmd = %08X);\r\n", command->cmdId);
		break;
	}

	if(m_resultPtr[0] != 0)
	{
		//Send response
		DEVCTLREPLY reply;
		reply.header.commandId = COMMANDID_DEVCTL;
		CopyHeader(reply.header, command->header);
		reply.result = 0;
		reply.unknown2 = 0;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(DEVCTLREPLY));
	}

	SendSifReply();
	return 1;
}

void CFileIoHandler2200::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
	reply.semaphoreId = command.semaphoreId;
	reply.resultPtr = command.resultPtr;
	reply.resultSize = command.resultSize;
}

void CFileIoHandler2200::SendPendingReply(uint8* ram)
{
	//Send response
	if(m_resultPtr[0] != 0)
	{
		memcpy(ram + m_resultPtr[0], m_pendingReply.buffer.data(), m_pendingReply.replySize);
	}
	SendSifReply();
	m_pendingReply.valid = false;
}

void CFileIoHandler2200::SendSifReply()
{
	size_t packetSize = sizeof(SIFCMDHEADER);
	uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
	auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
	memset(header, 0, sizeof(SIFCMDHEADER));
	header->commandId = 0x80000011;
	header->packetSize = packetSize;
	m_sifMan.SendPacket(callbackPacket, packetSize);
}
