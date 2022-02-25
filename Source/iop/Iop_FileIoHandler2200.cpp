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

#define IOCTL_HDD_ADDSUB 0x6801

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
	case COMMANDID_WRITE:
		*ret = InvokeWrite(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_SEEK:
		*ret = InvokeSeek(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_REMOVE:
		*ret = InvokeRemove(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_MKDIR:
		*ret = InvokeMkdir(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_RMDIR:
		*ret = InvokeRmdir(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DOPEN:
		*ret = InvokeDopen(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DCLOSE:
		*ret = InvokeDclose(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DREAD:
		*ret = InvokeDread(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_GETSTAT:
		*ret = InvokeGetStat(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_CHSTAT:
		*ret = InvokeChstat(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_FORMAT:
		*ret = InvokeFormat(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_CHDIR:
		*ret = InvokeChdir(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_SYNC:
		*ret = InvokeSync(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_MOUNT:
		*ret = InvokeMount(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_UMOUNT:
		*ret = InvokeUmount(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_SEEK64:
		*ret = InvokeSeek64(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_DEVCTL:
		*ret = InvokeDevctl(args, argsSize, ret, retSize, ram);
		break;
	case COMMANDID_IOCTL2:
		*ret = InvokeIoctl2(args, argsSize, ret, retSize, ram);
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

	PrepareGenericReply(ram, command->header, COMMANDID_OPEN, result);
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

uint32 CFileIoHandler2200::InvokeWrite(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 48);
	assert(retSize == 4);
	auto command = reinterpret_cast<WRITECOMMAND*>(args);

	assert(command->unalignedSize == 0);
	uint32 writeAddress = command->buffer & (PS2::EE_RAM_SIZE - 1);
	auto result = m_ioman->Write(command->fd, command->size, reinterpret_cast<const void*>(ram + writeAddress));

	PrepareGenericReply(ram, command->header, COMMANDID_WRITE, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeSeek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<SEEKCOMMAND*>(args);
	auto result = m_ioman->Seek(command->fd, command->offset, command->whence);

	PrepareGenericReply(ram, command->header, COMMANDID_SEEK, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeRemove(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<CCODECOMMAND*>(args);
	auto result = 0;

	CLog::GetInstance().Print(LOG_NAME, "Remove('%s');\r\n", command->path);

	PrepareGenericReply(ram, command->header, COMMANDID_REMOVE, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeMkdir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<MKDIRCOMMAND*>(args);
	auto result = m_ioman->Mkdir(command->dirName);

	PrepareGenericReply(ram, command->header, COMMANDID_MKDIR, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeRmdir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<CCODECOMMAND*>(args);
	auto result = 0;

	CLog::GetInstance().Print(LOG_NAME, "Rmdir('%s');\r\n", command->path);

	PrepareGenericReply(ram, command->header, COMMANDID_RMDIR, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeDopen(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<DOPENCOMMAND*>(args);
	auto result = m_ioman->Dopen(command->dirName);

	PrepareGenericReply(ram, command->header, COMMANDID_DOPEN, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeDclose(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 20);
	assert(retSize == 4);
	auto command = reinterpret_cast<DCLOSECOMMAND*>(args);
	auto result = m_ioman->Dclose(command->fd);

	PrepareGenericReply(ram, command->header, COMMANDID_DCLOSE, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeDread(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 32);
	assert(retSize == 4);
	auto command = reinterpret_cast<DREADCOMMAND*>(args);
	Ioman::DIRENTRY dirEntry = {};
	auto result = m_ioman->Dread(command->fd, &dirEntry);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		DREADREPLY reply;
		reply.header.commandId = COMMANDID_DREAD;
		CopyHeader(reply.header, command->header);
		reply.result = result;
		reply.dirEntryPtr = command->dirEntryPtr;
		memcpy(&reply.stat, &dirEntry.stat, sizeof(dirEntry.stat));
		memcpy(reply.name, dirEntry.name, Ioman::DIRENTRY::NAME_SIZE);
		memcpy(ram + m_resultPtr[0], &reply, sizeof(DREADREPLY));
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

uint32 CFileIoHandler2200::InvokeChstat(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<CHSTATCOMMAND*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Chstat('%s', %d);\r\n", command->path, command->flags);

	auto result = 0;

	PrepareGenericReply(ram, command->header, COMMANDID_CHSTAT, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeFormat(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0xC10);
	assert(retSize == 4);

	auto command = reinterpret_cast<FORMATCOMMAND*>(args);

	//This is a stub and is not implemented yet
	CLog::GetInstance().Print(LOG_NAME, "Format(device = '%s', blockDevice = '%s', args, argsSize = %d);\r\n",
	                          command->device, command->blockDevice, command->argsSize);

	PrepareGenericReply(ram, command->header, COMMANDID_FORMAT, 0);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeChdir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Gigawing Generations & FF11
	assert(argsSize >= 0xD);
	assert(retSize == 4);
	auto command = reinterpret_cast<CCODECOMMAND*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Chdir('%s');\r\n", command->path);

	PrepareGenericReply(ram, command->header, COMMANDID_CHDIR, 0);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeSync(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);
	assert(retSize == 4);
	auto command = reinterpret_cast<SYNCCOMMAND*>(args);

	//Seems deviceName can be either a string or a fd (FFX)
	CLog::GetInstance().Print(LOG_NAME, "Sync(...);\r\n");

	PrepareGenericReply(ram, command->header, COMMANDID_SYNC, 0);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeMount(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Final Fantasy X
	assert(argsSize >= 0xC14);
	assert(retSize == 4);
	auto command = reinterpret_cast<MOUNTCOMMAND*>(args);
	auto result = m_ioman->Mount(command->fileSystemName, command->deviceName);

	PrepareGenericReply(ram, command->header, COMMANDID_MOUNT, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeUmount(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Romancing Saga with 'pfs0:' parameter.
	assert(argsSize >= 0x0C);
	assert(retSize == 4);
	auto command = reinterpret_cast<UMOUNTCOMMAND*>(args);
	auto result = m_ioman->Umount(command->deviceName);

	PrepareGenericReply(ram, command->header, COMMANDID_UMOUNT, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeSeek64(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto command = reinterpret_cast<SEEK64COMMAND*>(args);
	auto result = m_ioman->Seek64(command->fd, command->offset, command->whence);

	//Send response
	if(m_resultPtr[0] != 0)
	{
		SEEK64REPLY reply;
		reply.header.commandId = COMMANDID_SEEK64;
		CopyHeader(reply.header, command->header);
		reply.result = result;
		reply.unknown3 = 0;
		reply.unknown4 = 0;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(SEEK64REPLY));
	}

	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeDevctl(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//This is used by Romancing Saga and FFX with 'hdd0:' parameter.
	//This is also used by Phantasy Star Collection & PS Universe with 'cdrom0:' parameter.

	assert(argsSize >= 0x81C);
	assert(retSize == 4);
	auto command = reinterpret_cast<DEVCTLCOMMAND*>(args);

	uint32* input = reinterpret_cast<uint32*>(command->inputBuffer);
	uint32* output = reinterpret_cast<uint32*>(ram + command->outputPtr);
	auto result = m_ioman->DevCtl(command->device, command->cmdId, input, command->inputSize, output, command->outputSize);

	PrepareGenericReply(ram, command->header, COMMANDID_DEVCTL, result);
	SendSifReply();
	return 1;
}

uint32 CFileIoHandler2200::InvokeIoctl2(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x420);
	assert(retSize == 4);
	auto command = reinterpret_cast<IOCTL2COMMAND*>(args);

	switch(command->cmdId)
	{
	case IOCTL_HDD_ADDSUB:
		CLog::GetInstance().Print(LOG_NAME, "IoCtl2 -> HddAddSub(%s);\r\n", command->inputBuffer);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "IoCtl2 -> Unknown(cmd = %08X);\r\n", command->cmdId);
		break;
	}

	PrepareGenericReply(ram, command->header, COMMANDID_IOCTL2, 0);
	SendSifReply();
	return 1;
}

void CFileIoHandler2200::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
	reply.semaphoreId = command.semaphoreId;
	reply.resultPtr = command.resultPtr;
	reply.resultSize = command.resultSize;
}

void CFileIoHandler2200::PrepareGenericReply(uint8* ram, const COMMANDHEADER& header, COMMANDID commandId, uint32 result)
{
	if(m_resultPtr[0] != 0)
	{
		GENERICREPLY reply;
		reply.header.commandId = commandId;
		CopyHeader(reply.header, header);
		reply.result = result;
		memcpy(ram + m_resultPtr[0], &reply, sizeof(GENERICREPLY));
	}
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
