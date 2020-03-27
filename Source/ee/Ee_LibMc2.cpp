#include <cassert>
#include "Ee_LibMc2.h"
#include "Ps2Const.h"
#include "Log.h"
#include "../iop/IopBios.h"
#include "../iop/Iop_McServ.h"

using namespace Ee;

#define LOG_NAME "ee_libmc2"

#define MC_PORT 1

#define SIGNALSEMA_SYSCALL 0x42
#define WAITSEMA_SYSCALL 0x44
#define POLLSEMA_SYSCALL 0x45

CLibMc2::CLibMc2(uint8* ram, CIopBios& iopBios)
	: m_ram(ram)
	, m_iopBios(iopBios)
{
	
}

uint32 CLibMc2::AnalyzeFunction(uint32 startAddress, int16 stackAlloc)
{
	static const uint32 maxFunctionSize = 0x200;
	bool completed = false;
	uint32 maxAddress = startAddress + maxFunctionSize;
	uint32 address = startAddress + 4;
	maxAddress = std::min<uint32>(maxAddress, PS2::EE_RAM_SIZE);

	//Pattern matching stats
	uint32 countLUI8101 = 0;
	std::vector<uint32> constantsLoaded;
	std::unordered_map<uint32, uint32> syscallsUsed;

	while(address < maxAddress)
	{
		uint32 opcode = *reinterpret_cast<uint32*>(m_ram + address);
		if((opcode & 0xFFFF0000) == 0x27BD0000)
		{
			int16 offset = static_cast<int16>(opcode);
			if(offset == stackAlloc)
			{
				//Ok, we're done
				completed = true;
				break;
			}
		}
		//Check LUI {r}, {i}
		else if((opcode & 0xFFE00000) == 0x3C000000)
		{
			uint32 imm = opcode & 0xFFFF;
			if(imm == 0x8101)
			{
				countLUI8101++;
			}
		}
		//Check for ADDIU R0, {r}, {i}
		else if((opcode & 0xFFE00000) == 0x24000000)
		{
			constantsLoaded.push_back(opcode & 0xFFFF);
		}
		//Check for JAL {a}
		else if((opcode & 0xFC000000) == 0x0C000000)
		{
			uint32 jmpAddr = (address & 0xF0000000) | ((opcode & 0x3FFFFFF) * 4);
			if(jmpAddr < PS2::EE_RAM_SIZE)
			{
				uint32 opAtJmp = *reinterpret_cast<uint32*>(m_ram + jmpAddr);
				//Check for ADDIU V1, R0, {imm}
				if((opAtJmp & 0xFFFF0000) == 0x24030000)
				{
					syscallsUsed[opAtJmp & 0xFFFF]++;
				}
			}
		}
		address += 4;
	}

	if(completed)
	{
		if(!constantsLoaded.empty() && (countLUI8101 != 0))
		{
			auto has0x20 = std::find(constantsLoaded.begin(), constantsLoaded.end(), 0x20);
			int i = 0;
			i++;
		}
		if(syscallsUsed.size() == 2)
		{
			uint32 signalSemaCount = (syscallsUsed.find(SIGNALSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[SIGNALSEMA_SYSCALL] : 0);
			uint32 waitSemaCount = (syscallsUsed.find(WAITSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[WAITSEMA_SYSCALL] : 0);
			uint32 pollSemaCount = (syscallsUsed.find(POLLSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[POLLSEMA_SYSCALL] : 0);
			if((waitSemaCount == 1) && (pollSemaCount == 1))
			{
				m_checkAsyncPtr = startAddress;
			}
			if((waitSemaCount == 1) && (signalSemaCount == 3) && (countLUI8101 != 0) && !constantsLoaded.empty())
			{
				switch(constantsLoaded[0])
				{
				case 0x02:
					m_getInfoAsyncPtr = startAddress;
					break;
				case 0x0A:
					m_getDirAsyncPtr = startAddress;
					break;
				case 0x0E:
					m_searchFileAsyncPtr = startAddress;
					break;
				case 0x20:
					m_readFileAsyncPtr = startAddress;
					break;
				}
			}
		}
		return address;
	}

	return startAddress;
}

void CLibMc2::HookLibMc2Functions()
{
	for(uint32 address = 0; address < PS2::EE_RAM_SIZE; address += 4)
	{
		uint32 opcode = *reinterpret_cast<uint32*>(m_ram + address);
		//Look for ADDIU SP, SP, {i}
		if((opcode & 0xFFFF0000) == 0x27BD0000)
		{
			int16 offset = static_cast<int16>(opcode);
			if(offset < 0)
			{
				//Might be a function start
				address = AnalyzeFunction(address, -offset);
			}
		}
	}

	WriteSyscall(m_getInfoAsyncPtr, SYSCALL_MC2_GETINFO_ASYNC);
	WriteSyscall(m_getDirAsyncPtr, SYSCALL_MC2_GETDIR_ASYNC);
	WriteSyscall(m_searchFileAsyncPtr, SYSCALL_MC2_SEARCHFILE_ASYNC);
	WriteSyscall(m_readFileAsyncPtr, SYSCALL_MC2_READFILE_ASYNC);
	WriteSyscall(m_checkAsyncPtr, SYSCALL_MC2_CHECKASYNC);
}

void CLibMc2::WriteSyscall(uint32 address, uint16 syscallNumber)
{
	assert(address != 0);
	if(address == 0) return;

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
	cmd.port = MC_PORT;
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
	cmd.port = MC_PORT;
	cmd.maxEntries = 1;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	std::vector<Iop::CMcServ::ENTRY> entries;
	if(cmd.maxEntries > 0)
	{
		entries.resize(cmd.maxEntries);
	}
	mcServ->Invoke(0xD, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), &result, sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

	if(static_cast<int32>(result) < 0)
	{
		m_lastResult = 0x81010014;
	}
	else
	{
		memset(dirParam, 0, sizeof(DIRPARAM));
		dirParam->attributes = entries[0].attributes;
		dirParam->size = entries[0].size;
		strcpy(dirParam->name, reinterpret_cast<char*>(entries[0].name));

		m_lastResult = 0x81010000;

		assert(result == 1);
	}

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
		cmd.port = MC_PORT;
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
