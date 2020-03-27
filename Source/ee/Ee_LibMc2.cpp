#include <cassert>
#include "Ee_LibMc2.h"
#include "Log.h"
#include "../iop/IopBios.h"
#include "../iop/Iop_McServ.h"

using namespace Ee;

#define LOG_NAME "ee_libmc2"

CLibMc2::CLibMc2(uint8* ram, CIopBios& iopBios)
	: m_ram(ram)
	, m_iopBios(iopBios)
{
	
}

void CLibMc2::HookLibMc2Functions()
{
#if 0
	WriteSyscall(0x005F27F0, SYSCALL_MC2_CHECKASYNC);
	WriteSyscall(0x005F15E8, SYSCALL_MC2_GETINFO_ASYNC);
	WriteSyscall(0x005F1EB8, SYSCALL_MC2_GETDIR_ASYNC);
	WriteSyscall(0x005F16A8, SYSCALL_MC2_SEARCHFILE_ASYNC);
	WriteSyscall(0x005F1978, SYSCALL_MC2_READFILE_ASYNC);
#endif
}

void CLibMc2::WriteSyscall(uint32 address, uint16 syscallNumber)
{
	*reinterpret_cast<uint32*>(m_ram + address + 0x0) = 0x24030000 | syscallNumber;
	*reinterpret_cast<uint32*>(m_ram + address + 0x4) = 0x0000000C;
	*reinterpret_cast<uint32*>(m_ram + address + 0x8) = 0x03E00008;
	*reinterpret_cast<uint32*>(m_ram + address + 0xC) = 0x00000000;
}

void CLibMc2::HandleSyscall(CMIPS& ee)
{
	switch(ee.m_State.nGPR[CMIPS::V1].nV0)
	{
	case SYSCALL_MC2_CHECKASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = CheckAsync(
			ee.m_State.nGPR[CMIPS::A0].nV0,
			ee.m_State.nGPR[CMIPS::A1].nV0,
			ee.m_State.nGPR[CMIPS::A2].nV0
		);
		break;
	case SYSCALL_MC2_GETINFO_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetInfoAsync(
			ee.m_State.nGPR[CMIPS::A0].nV0,
			ee.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case SYSCALL_MC2_GETDIR_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetDirAsync(
			ee.m_State.nGPR[CMIPS::A0].nV0,
			ee.m_State.nGPR[CMIPS::A1].nV0,
			ee.m_State.nGPR[CMIPS::A2].nV0,
			ee.m_State.nGPR[CMIPS::A3].nV0,
			ee.m_State.nGPR[CMIPS::T0].nV0,
			ee.m_State.nGPR[CMIPS::T1].nV0
		);
		break;
	case SYSCALL_MC2_SEARCHFILE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = SearchFileAsync(
			ee.m_State.nGPR[CMIPS::A0].nV0,
			ee.m_State.nGPR[CMIPS::A1].nV0,
			ee.m_State.nGPR[CMIPS::A2].nV0
		);
		break;
	case SYSCALL_MC2_READFILE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = ReadFileAsync(
			ee.m_State.nGPR[CMIPS::A0].nV0,
			ee.m_State.nGPR[CMIPS::A1].nV0,
			ee.m_State.nGPR[CMIPS::A2].nV0,
			ee.m_State.nGPR[CMIPS::A3].nV0,
			ee.m_State.nGPR[CMIPS::T0].nV0
		);
		break;
	default:
		assert(false);
		break;
	}
}

int32 CLibMc2::CheckAsync(uint32 mode, uint32 cmdPtr, uint32 resultPtr)
{
	CLog::GetInstance().Print(LOG_NAME, "CheckAsync(mode = %d, cmdPtr = 0x%08X, resultPtr = 0x%08X);\r\n",
		mode, cmdPtr, resultPtr);

	assert(m_lastCmd != 0);

	*reinterpret_cast<uint32*>(m_ram + cmdPtr) = m_lastCmd;
	*reinterpret_cast<uint32*>(m_ram + resultPtr) = m_lastResult;

	m_lastCmd = 0;

	//Returns 1 when function has completed execution
	return 1;
}

int32 CLibMc2::GetInfoAsync(uint32 socketId, uint32 infoPtr)
{
	auto info = reinterpret_cast<CARDINFO*>(m_ram + infoPtr);

	CLog::GetInstance().Print(LOG_NAME, "GetInfoAsync(socketId = %d, infoPtr = 0x%08X);\r\n",
		socketId, infoPtr);

	info->type = 2; //2 = PS2
	info->formatted = 1;
	info->freeClusters = 0x1E81;

	//Potential return value 0x81019003 -> Probably means memory card changed
	m_lastResult = 0x81010000;
	m_lastCmd = SYSCALL_MC2_GETINFO_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::GetDirAsync(uint32 socketId, uint32 pathPtr, uint32 offset, int32 maxEntries, uint32 dirEntriesPtr, uint32 countPtr)
{
	auto path = reinterpret_cast<const char*>(m_ram + pathPtr);
	auto dirEntries = reinterpret_cast<DIRPARAM*>(m_ram + dirEntriesPtr);

	CLog::GetInstance().Print(LOG_NAME, "GetDirAsync(socketId = %d, path = '%s', offset = %d, maxEntries = %d, dirEntriesPtr = 0x%08X, countPtr = 0x%08X);\r\n",
		socketId, path, offset, maxEntries, dirEntriesPtr, countPtr);

	auto mcServ = m_iopBios.GetMcServ();

	uint32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = 1;
	cmd.maxEntries = maxEntries;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	std::vector<Iop::CMcServ::ENTRY> entries;
	if(cmd.maxEntries > 0)
	{
		entries.resize(cmd.maxEntries);
	}
	mcServ->Invoke(0xD, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

	*reinterpret_cast<uint32*>(m_ram + countPtr) = result;

	if((result != 0) && (maxEntries > 0))
	{
		auto dirParam = dirEntries;
		for(uint32 i = 0; i < result; i++)
		{
			memset(dirParam, 0, sizeof(DIRPARAM));
			dirParam->attributes = entries[i].attributes;
			dirParam->size = entries[i].size;
			strcpy(dirParam->name, reinterpret_cast<char*>(entries[i].name));
			dirParam++;
		}
	}

	m_lastResult = 0x81010000;
	m_lastCmd = 0xC;

	return 0;
}

int32 CLibMc2::SearchFileAsync(uint32 socketId, uint32 pathPtr, uint32 dirParamPtr)
{
	auto path = reinterpret_cast<const char*>(m_ram + pathPtr);
	auto dirParam = reinterpret_cast<DIRPARAM*>(m_ram + dirParamPtr);

	CLog::GetInstance().Print(LOG_NAME, "SearchFileAsync(socketId = %d, path = '%s', dirParamPtr = 0x%08X);\r\n",
		socketId, path, dirParamPtr);

	auto mcServ = m_iopBios.GetMcServ();

	uint32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = 1;
	cmd.maxEntries = 1;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	std::vector<Iop::CMcServ::ENTRY> entries;
	if(cmd.maxEntries > 0)
	{
		entries.resize(cmd.maxEntries);
	}
	mcServ->Invoke(0xD, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

	assert(result == 1);

	memset(dirParam, 0, sizeof(DIRPARAM));
	dirParam->attributes = entries[0].attributes;
	dirParam->size = entries[0].size;
	strcpy(dirParam->name, reinterpret_cast<char*>(entries[0].name));

	m_lastResult = 0x81010000;
	m_lastCmd = SYSCALL_MC2_SEARCHFILE_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::ReadFileAsync(uint32 socketId, uint32 pathPtr, uint32 bufferPtr, uint32 offset, uint32 size)
{
	auto path = reinterpret_cast<const char*>(m_ram + pathPtr);

	CLog::GetInstance().Print(LOG_NAME, "ReadFileAsync(socketId = %d, path = '%s', bufferPtr = 0x%08X, offset = 0x%08X, size = 0x%08X);\r\n",
		socketId, path, bufferPtr, offset, size);

	auto mcServ = m_iopBios.GetMcServ();

	uint32 fd = 0;

	{
		//Issue open command
		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.flags = Iop::CMcServ::OPEN_FLAG_RDONLY;
		cmd.port = 1;
		assert(strlen(path) <= sizeof(cmd.name));
		strncpy(cmd.name, path, sizeof(cmd.name));
		mcServ->Invoke(Iop::CMcServ::CMD_ID_OPEN, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &fd, sizeof(uint32), nullptr);
	}

	assert(fd >= 0);

	if(offset != 0)
	{
		uint32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.offset = offset;
		cmd.origin = 0;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_SEEK, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), nullptr);
		assert(result == offset);
	}

	//Read
	{
		uint32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.size = size;
		cmd.bufferAddress = bufferPtr;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_READ, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), m_ram);
		assert(result >= 0);
	}

	//Close
	{
		uint32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_CLOSE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), nullptr);
		assert(result >= 0);
	}

	m_lastResult = size;
	m_lastCmd = SYSCALL_MC2_READFILE_ASYNC & 0xFF;

	return 0;
}
