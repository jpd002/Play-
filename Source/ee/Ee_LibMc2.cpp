#include "Ee_LibMc2.h"
#include <cassert>
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "Ps2Const.h"
#include "Log.h"
#include "PS2OS.h"
#include "../iop/Iop_McServ.h"

using namespace Ee;

#define LOG_NAME "ee_libmc2"

#define STATE_XML ("libmc2/state.xml")
#define STATE_LAST_CMD ("lastCmd")
#define STATE_LAST_RESULT ("lastResult")
#define STATE_WAIT_THREADID ("waitThreadId")
#define STATE_WAIT_VBLANK_COUNT ("waitVBlankCount")

#define MC_PORT 0

#define SIGNALSEMA_SYSCALL 0x42
#define WAITSEMA_SYSCALL 0x44
#define POLLSEMA_SYSCALL 0x45

//Really not sure about these
#define MC2_RESULT_OK 0
#define MC2_RESULT_ERROR_NOT_FOUND 0x81010002

CLibMc2::CLibMc2(uint8* ram, CPS2OS& eeBios, CIopBios& iopBios)
    : m_ram(ram)
    , m_eeBios(eeBios)
    , m_iopBios(iopBios)
{
	m_moduleLoadedConnection = m_iopBios.OnModuleLoaded.Connect(
	    [this](const char* moduleName) { OnIopModuleLoaded(moduleName); });
}

void CLibMc2::Reset()
{
	m_lastCmd = 0;
	m_lastResult = 0;
	m_waitThreadId = WAIT_THREAD_ID_EMPTY;
	m_waitVBlankCount = 0;
}

void CLibMc2::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto registerFile = new CRegisterStateFile(STATE_XML);
	registerFile->SetRegister32(STATE_LAST_CMD, m_lastCmd);
	registerFile->SetRegister32(STATE_LAST_RESULT, m_lastResult);
	registerFile->SetRegister32(STATE_WAIT_THREADID, m_waitThreadId);
	registerFile->SetRegister32(STATE_WAIT_VBLANK_COUNT, m_waitVBlankCount);
	archive.InsertFile(registerFile);
}

void CLibMc2::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_XML));
	m_lastCmd = registerFile.GetRegister32(STATE_LAST_CMD);
	m_lastResult = registerFile.GetRegister32(STATE_LAST_RESULT);
	m_waitThreadId = registerFile.GetRegister32(STATE_WAIT_THREADID);
	m_waitVBlankCount = registerFile.GetRegister32(STATE_WAIT_VBLANK_COUNT);
}

uint32 CLibMc2::AnalyzeFunction(MODULE_FUNCTIONS& moduleFunctions, uint32 startAddress, int16 stackAlloc)
{
	static const uint32 maxFunctionSize = 0x200;
	bool completed = false;
	uint32 maxAddress = startAddress + maxFunctionSize;
	uint32 address = startAddress + 4;
	maxAddress = std::min<uint32>(maxAddress, PS2::EE_RAM_SIZE);

	//Pattern matching stats
	uint32 countLUI8101 = 0;
	uint32 jalCount = 0;
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
			jalCount++;
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
		if((countLUI8101 > 2) && (constantsLoaded.size() == 1))
		{
			switch(constantsLoaded[0])
			{
			case 0x02:
				moduleFunctions.getInfoAsyncPtr = startAddress;
				break;
			case 0x05:
				moduleFunctions.readFileAsyncPtr = startAddress;
				break;
			case 0x06:
				moduleFunctions.writeFileAsyncPtr = startAddress;
				break;
			case 0x07:
				moduleFunctions.createFileAsyncPtr = startAddress;
				break;
			case 0x08:
				moduleFunctions.deleteAsyncPtr = startAddress;
				break;
			case 0x0A:
				if(stackAlloc < 0x100)
				{
					//Mana Khemia 2 has 2 potential functions that matches,
					//we use the stack alloc to make sure we get the right one
					moduleFunctions.getDirAsyncPtr = startAddress;
				}
				break;
			case 0x0B:
				moduleFunctions.mkDirAsyncPtr = startAddress;
				break;
			case 0x0C:
				moduleFunctions.chDirAsyncPtr = startAddress;
				break;
			case 0x0D:
				moduleFunctions.chModAsyncPtr = startAddress;
				break;
			case 0x0E:
				moduleFunctions.searchFileAsyncPtr = startAddress;
				break;
			case 0x0F:
				moduleFunctions.getEntSpaceAsyncPtr = startAddress;
				break;
			case 0x20:
				moduleFunctions.readFile2AsyncPtr = startAddress;
				break;
			case 0x21:
				moduleFunctions.writeFile2AsyncPtr = startAddress;
				break;
			}
		}
		if(syscallsUsed.size() == 2 && (jalCount == 2))
		{
			uint32 signalSemaCount = (syscallsUsed.find(SIGNALSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[SIGNALSEMA_SYSCALL] : 0);
			uint32 waitSemaCount = (syscallsUsed.find(WAITSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[WAITSEMA_SYSCALL] : 0);
			uint32 pollSemaCount = (syscallsUsed.find(POLLSEMA_SYSCALL) != std::end(syscallsUsed) ? syscallsUsed[POLLSEMA_SYSCALL] : 0);
			if((waitSemaCount == 1) && (pollSemaCount == 1))
			{
				moduleFunctions.checkAsyncPtr = startAddress;
			}
		}
		if((countLUI8101 == 3) && (jalCount == 2) && (syscallsUsed.size() == 0) && (constantsLoaded.size() == 0))
		{
			moduleFunctions.getDbcStatusPtr = startAddress;
		}
		return address;
	}

	return startAddress;
}

void CLibMc2::OnIopModuleLoaded(const char* moduleName)
{
	if(
	    !strcmp(moduleName, "mc2_d ") ||
	    !strcmp(moduleName, "mc2_s1"))
	{
		HookLibMc2Functions();
	}
}

void CLibMc2::HookLibMc2Functions()
{
	MODULE_FUNCTIONS moduleFunctions;

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
				address = AnalyzeFunction(moduleFunctions, address, -offset);
			}
		}
	}

	WriteSyscall(moduleFunctions.getInfoAsyncPtr, SYSCALL_MC2_GETINFO_ASYNC);
	WriteSyscall(moduleFunctions.readFileAsyncPtr, SYSCALL_MC2_READFILE_ASYNC);
	WriteSyscall(moduleFunctions.writeFileAsyncPtr, SYSCALL_MC2_WRITEFILE_ASYNC);
	WriteSyscall(moduleFunctions.createFileAsyncPtr, SYSCALL_MC2_CREATEFILE_ASYNC);
	WriteSyscall(moduleFunctions.deleteAsyncPtr, SYSCALL_MC2_DELETE_ASYNC);
	WriteSyscall(moduleFunctions.getDirAsyncPtr, SYSCALL_MC2_GETDIR_ASYNC);
	WriteSyscall(moduleFunctions.mkDirAsyncPtr, SYSCALL_MC2_MKDIR_ASYNC);
	WriteSyscall(moduleFunctions.chDirAsyncPtr, SYSCALL_MC2_CHDIR_ASYNC);
	WriteSyscall(moduleFunctions.chModAsyncPtr, SYSCALL_MC2_CHMOD_ASYNC);
	WriteSyscall(moduleFunctions.searchFileAsyncPtr, SYSCALL_MC2_SEARCHFILE_ASYNC);
	WriteSyscall(moduleFunctions.getEntSpaceAsyncPtr, SYSCALL_MC2_GETENTSPACE_ASYNC);
	WriteSyscall(moduleFunctions.readFile2AsyncPtr, SYSCALL_MC2_READFILE2_ASYNC);
	WriteSyscall(moduleFunctions.writeFile2AsyncPtr, SYSCALL_MC2_WRITEFILE2_ASYNC);
	WriteSyscall(moduleFunctions.checkAsyncPtr, SYSCALL_MC2_CHECKASYNC);
	WriteSyscall(moduleFunctions.getDbcStatusPtr, SYSCALL_MC2_GETDBCSTATUS);
}

void CLibMc2::WriteSyscall(uint32 address, uint16 syscallNumber)
{
	if(address == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Implementation for %s not found.\r\n", GetSysCallDescription(syscallNumber));
		return;
	}

	CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + address));

	assembler.ADDIU(CMIPS::V1, CMIPS::R0, syscallNumber);
	assembler.SYSCALL();
	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

const char* CLibMc2::GetSysCallDescription(uint16 syscallNumber)
{
	switch(syscallNumber)
	{
	case SYSCALL_MC2_CHECKASYNC:
		return "CheckAsync";
	case SYSCALL_MC2_GETINFO_ASYNC:
		return "GetInfoAsync";
	case SYSCALL_MC2_READFILE_ASYNC:
		return "ReadFileAsync";
	case SYSCALL_MC2_WRITEFILE_ASYNC:
		return "WriteAsync";
	case SYSCALL_MC2_CREATEFILE_ASYNC:
		return "CreateFileAsync";
	case SYSCALL_MC2_DELETE_ASYNC:
		return "DeleteAsync";
	case SYSCALL_MC2_GETDIR_ASYNC:
		return "GetDirAsync";
	case SYSCALL_MC2_MKDIR_ASYNC:
		return "MkDirAsync";
	case SYSCALL_MC2_CHDIR_ASYNC:
		return "ChDirAsync";
	case SYSCALL_MC2_CHMOD_ASYNC:
		return "ChModAsync";
	case SYSCALL_MC2_SEARCHFILE_ASYNC:
		return "SearchFileAsync";
	case SYSCALL_MC2_GETENTSPACE_ASYNC:
		return "GetEntSpaceAsync";
	case SYSCALL_MC2_READFILE2_ASYNC:
		return "ReadFile2Async";
	case SYSCALL_MC2_WRITEFILE2_ASYNC:
		return "WriteFile2Async";
	case SYSCALL_MC2_GETDBCSTATUS:
		return "GetDbcStatus";
	default:
		return "unknown";
		break;
	}
}

void CLibMc2::HandleSyscall(CMIPS& ee)
{
	switch(ee.m_State.nGPR[CMIPS::V1].nV0)
	{
	case SYSCALL_MC2_CHECKASYNC:
		CheckAsync(ee);
		break;
	case SYSCALL_MC2_GETINFO_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetInfoAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case SYSCALL_MC2_READFILE_ASYNC:
	case SYSCALL_MC2_READFILE2_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = ReadFileAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0,
		    ee.m_State.nGPR[CMIPS::A3].nV0,
		    ee.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case SYSCALL_MC2_WRITEFILE_ASYNC:
	case SYSCALL_MC2_WRITEFILE2_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = WriteFileAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0,
		    ee.m_State.nGPR[CMIPS::A3].nV0,
		    ee.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case SYSCALL_MC2_CREATEFILE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = CreateFileAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case SYSCALL_MC2_DELETE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = DeleteAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case SYSCALL_MC2_GETDIR_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetDirAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0,
		    ee.m_State.nGPR[CMIPS::A3].nV0,
		    ee.m_State.nGPR[CMIPS::T0].nV0,
		    ee.m_State.nGPR[CMIPS::T1].nV0);
		break;
	case SYSCALL_MC2_MKDIR_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = MkDirAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case SYSCALL_MC2_CHDIR_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = ChDirAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case SYSCALL_MC2_CHMOD_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = ChModAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case SYSCALL_MC2_SEARCHFILE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = SearchFileAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0,
		    ee.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case SYSCALL_MC2_GETENTSPACE_ASYNC:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetEntSpaceAsync(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case SYSCALL_MC2_GETDBCSTATUS:
		ee.m_State.nGPR[CMIPS::V0].nD0 = GetDbcStatus(
		    ee.m_State.nGPR[CMIPS::A0].nV0,
		    ee.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		assert(false);
		break;
	}
}

void CLibMc2::NotifyVBlankStart()
{
	if(m_waitThreadId != WAIT_THREAD_ID_EMPTY)
	{
		assert(m_waitVBlankCount != 0);
		m_waitVBlankCount--;
		if(m_waitVBlankCount == 0)
		{
			m_eeBios.ResumeThread(m_waitThreadId);
			m_waitThreadId = WAIT_THREAD_ID_EMPTY;
		}
	}
}

static void CopyDirParamTime(CLibMc2::DIRPARAM::TIME* dst, const Iop::CMcServ::ENTRY::TIME* src)
{
	dst->year = src->year;
	dst->month = src->month;
	dst->day = src->day;
	dst->hour = src->hour;
	dst->minute = src->minute;
	dst->second = src->second;
}

static void CopyDirParam(CLibMc2::DIRPARAM* dst, const Iop::CMcServ::ENTRY* src)
{
	dst->attributes = src->attributes;
	dst->size = src->size;
	strcpy(dst->name, reinterpret_cast<const char*>(src->name));
	CopyDirParamTime(&dst->creationDate, &src->creationTime);
	CopyDirParamTime(&dst->modificationDate, &src->modificationTime);
}

void CLibMc2::CheckAsync(CMIPS& context)
{
	uint32 mode = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 cmdPtr = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 resultPtr = context.m_State.nGPR[CMIPS::A2].nV0;

	CLog::GetInstance().Print(LOG_NAME, "CheckAsync(mode = %d, cmdPtr = 0x%08X, resultPtr = 0x%08X);\r\n",
	                          mode, cmdPtr, resultPtr);

	assert(m_lastCmd != 0);

	//Returns 1 when function has completed execution
	//Returns -1 if no function was executing
	uint32 result = (m_lastCmd != 0) ? 1 : -1;

	//Don't report last cmd result if we didn't execute a command
	uint32 lastCmdResult = (m_lastCmd != 0) ? m_lastResult : 0;

	if(cmdPtr != 0)
	{
		*reinterpret_cast<uint32*>(m_eeBios.GetStructPtr(cmdPtr)) = m_lastCmd;
	}
	if(resultPtr != 0)
	{
		*reinterpret_cast<uint32*>(m_eeBios.GetStructPtr(resultPtr)) = lastCmdResult;
	}

	m_lastCmd = 0;

	context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(result);

	//Mode:
	//0 -> Sync (use WaitSema)
	//1 -> Async (use PollSema)
	if(mode == 0)
	{
		//In Sync Mode, sleep thread for a few vblanks
		assert(m_waitThreadId == WAIT_THREAD_ID_EMPTY);
		m_waitVBlankCount = WAIT_VBLANK_INIT_COUNT;
		m_waitThreadId = m_eeBios.SuspendCurrentThread();
	}
}

int32 CLibMc2::GetInfoAsync(uint32 socketId, uint32 infoPtr)
{
	auto info = reinterpret_cast<CARDINFO*>(m_eeBios.GetStructPtr(infoPtr));

	CLog::GetInstance().Print(LOG_NAME, "GetInfoAsync(socketId = %d, infoPtr = 0x%08X);\r\n",
	                          socketId, infoPtr);

	info->type = 2; //2 = PS2
	info->formatted = 1;
	info->freeClusters = 0x1E81;

	//Potential return value 0x81019003 -> Probably means memory card changed
	m_lastResult = MC2_RESULT_OK;
	m_lastCmd = SYSCALL_MC2_GETINFO_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::CreateFileAsync(uint32 socketId, uint32 pathPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "CreateFileAsync(socketId = %d, path = '%s');\r\n",
	                          socketId, path);

	auto mcServ = m_iopBios.GetMcServ();

	int32 fd = 0;

	{
		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.port = MC_PORT;
		cmd.flags = Iop::CMcServ::OPEN_FLAG_CREAT;
		assert(strlen(path) <= sizeof(cmd.name));
		strncpy(cmd.name, path, sizeof(cmd.name));

		mcServ->Invoke(Iop::CMcServ::CMD_ID_OPEN, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&fd), sizeof(uint32), nullptr);

		assert(fd >= 0);
	}

	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;

		mcServ->Invoke(Iop::CMcServ::CMD_ID_CLOSE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);

		assert(result >= 0);
	}

	m_lastResult = MC2_RESULT_OK;
	m_lastCmd = SYSCALL_MC2_CREATEFILE_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::DeleteAsync(uint32 socketId, uint32 pathPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "DeleteAsync(socketId = %d, path = '%s');\r\n",
	                          socketId, path);

	auto mcServ = m_iopBios.GetMcServ();
	int32 result = 0;

	{
		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.port = MC_PORT;
		assert(strlen(path) <= sizeof(cmd.name));
		strncpy(cmd.name, path, sizeof(cmd.name));

		mcServ->Invoke(Iop::CMcServ::CMD_ID_DELETE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(int32), nullptr);
	}

	if(result >= 0)
	{
		m_lastResult = MC2_RESULT_OK;
	}
	else
	{
		assert(result == Iop::CMcServ::RET_NO_ENTRY);
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
	}

	m_lastCmd = SYSCALL_MC2_DELETE_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::GetDirAsync(uint32 socketId, uint32 pathPtr, uint32 offset, int32 maxEntries, uint32 dirEntriesPtr, uint32 countPtr)
{
	assert((maxEntries >= 0) || (offset == 0));

	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));
	auto dirEntries = reinterpret_cast<DIRPARAM*>(m_eeBios.GetStructPtr(dirEntriesPtr));

	CLog::GetInstance().Print(LOG_NAME, "GetDirAsync(socketId = %d, path = '%s', offset = %d, maxEntries = %d, dirEntriesPtr = 0x%08X, countPtr = 0x%08X);\r\n",
	                          socketId, path, offset, maxEntries, dirEntriesPtr, countPtr);

	auto mcServ = m_iopBios.GetMcServ();

	int32 entriesToFetch = (maxEntries >= 0) ? (maxEntries + offset) : maxEntries;

	int32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = MC_PORT;
	cmd.maxEntries = entriesToFetch;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	std::vector<Iop::CMcServ::ENTRY> entries;
	if(entriesToFetch >= 0)
	{
		entries.resize(entriesToFetch);
	}
	mcServ->Invoke(Iop::CMcServ::CMD_ID_GETDIR, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

	if(result < 0)
	{
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
	}
	else
	{
		if(maxEntries < 0)
		{
			//Wanted to get the total amount of entries
			*reinterpret_cast<uint32*>(m_eeBios.GetStructPtr(countPtr)) = result;
		}
		else
		{
			assert(result >= offset);
			*reinterpret_cast<uint32*>(m_eeBios.GetStructPtr(countPtr)) = result - offset;

			auto dirParam = dirEntries;
			for(uint32 i = offset; i < result; i++)
			{
				memset(dirParam, 0, sizeof(DIRPARAM));
				CopyDirParam(dirParam, &entries[i]);
				dirParam++;
			}
		}

		m_lastResult = MC2_RESULT_OK;
	}

	m_lastCmd = SYSCALL_MC2_GETDIR_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::MkDirAsync(uint32 socketId, uint32 pathPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "MkDirAsync(socketId = %d, path = '%s');\r\n",
	                          socketId, path);

	auto mcServ = m_iopBios.GetMcServ();

	int32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = MC_PORT;
	cmd.flags = 0x40;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	mcServ->Invoke(Iop::CMcServ::CMD_ID_OPEN, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);

	assert(result >= 0);

	m_lastResult = MC2_RESULT_OK;
	m_lastCmd = SYSCALL_MC2_MKDIR_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::ChDirAsync(uint32 socketId, uint32 pathPtr, uint32 pwdPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));
	assert(pwdPtr == 0);

	CLog::GetInstance().Print(LOG_NAME, "ChDirAsync(socketId = %d, path = '%s', pwdPtr = 0x%08X);\r\n",
	                          socketId, path, pwdPtr);

	auto mcServ = m_iopBios.GetMcServ();

	int32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = MC_PORT;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	mcServ->Invoke(Iop::CMcServ::CMD_ID_CHDIR, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);

	if(result < 0)
	{
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
	}
	else
	{
		m_lastResult = MC2_RESULT_OK;
	}

	m_lastCmd = SYSCALL_MC2_CHDIR_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::ChModAsync(uint32 socketId, uint32 pathPtr, uint32 mode)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "ChModAsync(socketId = %d, path = '%s', mode = %d);\r\n",
	                          socketId, path, mode);

	m_lastResult = MC2_RESULT_OK;
	m_lastCmd = SYSCALL_MC2_CHMOD_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::SearchFileAsync(uint32 socketId, uint32 pathPtr, uint32 dirParamPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));
	auto dirParam = reinterpret_cast<DIRPARAM*>(m_eeBios.GetStructPtr(dirParamPtr));

	CLog::GetInstance().Print(LOG_NAME, "SearchFileAsync(socketId = %d, path = '%s', dirParamPtr = 0x%08X);\r\n",
	                          socketId, path, dirParamPtr);

	auto mcServ = m_iopBios.GetMcServ();

	int32 result = 0;
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
	mcServ->Invoke(Iop::CMcServ::CMD_ID_GETDIR, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), reinterpret_cast<uint8*>(entries.data()));

	if(result <= 0)
	{
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
	}
	else
	{
		assert(result == 1);

		memset(dirParam, 0, sizeof(DIRPARAM));
		CopyDirParam(dirParam, &entries[0]);

		m_lastResult = MC2_RESULT_OK;
	}

	m_lastCmd = SYSCALL_MC2_SEARCHFILE_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::GetEntSpaceAsync(uint32 socketId, uint32 pathPtr)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "GetEntSpaceAsync(socketId = %d, path = '%s');\r\n",
	                          socketId, path);

	auto mcServ = m_iopBios.GetMcServ();

	int32 result = 0;
	Iop::CMcServ::CMD cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.port = MC_PORT;
	assert(strlen(path) <= sizeof(cmd.name));
	strncpy(cmd.name, path, sizeof(cmd.name));

	mcServ->Invoke(Iop::CMcServ::CMD_ID_GETENTSPACE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);

	if(result < 0)
	{
		assert(result == Iop::CMcServ::RET_NO_ENTRY);
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
	}
	else
	{
		m_lastResult = result;
	}

	m_lastCmd = SYSCALL_MC2_GETENTSPACE_ASYNC & 0xFF;

	return 0;
}

int32 CLibMc2::GetDbcStatus(uint32 socketId, uint32 unknown)
{
	CLog::GetInstance().Print(LOG_NAME, "GetDbcStatus(socketId = %d, unknown = %d);\r\n",
	                          socketId, unknown);
	//Don't really know what that function does, but returning 0 seems to be the way to go
	return 0;
}

int32 CLibMc2::ReadFileAsync(uint32 socketId, uint32 pathPtr, uint32 bufferPtr, uint32 offset, uint32 size)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "ReadFileAsync(socketId = %d, path = '%s', bufferPtr = 0x%08X, offset = 0x%08X, size = 0x%08X);\r\n",
	                          socketId, path, bufferPtr, offset, size);

	auto mcServ = m_iopBios.GetMcServ();

	m_lastCmd = SYSCALL_MC2_READFILE_ASYNC & 0xFF;

	int32 fd = 0;

	{
		//Issue open command
		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.flags = Iop::CMcServ::OPEN_FLAG_RDONLY;
		cmd.port = MC_PORT;
		assert(strlen(path) <= sizeof(cmd.name));
		strncpy(cmd.name, path, sizeof(cmd.name));
		mcServ->Invoke(Iop::CMcServ::CMD_ID_OPEN, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&fd), sizeof(uint32), nullptr);
	}

	if(fd < 0)
	{
		assert(fd == Iop::CMcServ::RET_NO_ENTRY);
		m_lastResult = MC2_RESULT_ERROR_NOT_FOUND;
		return 0;
	}

	if(offset != 0)
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.offset = offset;
		cmd.origin = 0;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_SEEK, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);
		assert(result == offset);
	}

	//Read
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.size = size;
		cmd.bufferAddress = bufferPtr;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_READ, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), m_ram);
		assert(result >= 0);
	}

	//Close
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_CLOSE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);
		assert(result >= 0);
	}

	m_lastResult = size;

	return 0;
}

int32 CLibMc2::WriteFileAsync(uint32 socketId, uint32 pathPtr, uint32 bufferPtr, uint32 offset, uint32 size)
{
	auto path = reinterpret_cast<const char*>(m_eeBios.GetStructPtr(pathPtr));

	CLog::GetInstance().Print(LOG_NAME, "WriteFileAsync(socketId = %d, path = '%s', bufferPtr = 0x%08X, offset = 0x%08X, size = 0x%08X);\r\n",
	                          socketId, path, bufferPtr, offset, size);

	auto mcServ = m_iopBios.GetMcServ();

	int32 fd = 0;

	{
		//Issue open command
		Iop::CMcServ::CMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.flags = Iop::CMcServ::OPEN_FLAG_WRONLY;
		cmd.port = MC_PORT;
		assert(strlen(path) <= sizeof(cmd.name));
		strncpy(cmd.name, path, sizeof(cmd.name));
		mcServ->Invoke(Iop::CMcServ::CMD_ID_OPEN, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&fd), sizeof(uint32), nullptr);
	}

	assert(fd >= 0);

	if(offset != 0)
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.offset = offset;
		cmd.origin = 0;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_SEEK, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);
		assert(result == offset);
	}

	//Write
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		cmd.size = size;
		cmd.bufferAddress = bufferPtr;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_WRITE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), m_ram);
		assert(result >= 0);
	}

	//Close
	{
		int32 result = 0;
		Iop::CMcServ::FILECMD cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.handle = fd;
		mcServ->Invoke(Iop::CMcServ::CMD_ID_CLOSE, reinterpret_cast<uint32*>(&cmd), sizeof(cmd), reinterpret_cast<uint32*>(&result), sizeof(uint32), nullptr);
		assert(result >= 0);
	}

	m_lastResult = size;
	m_lastCmd = SYSCALL_MC2_WRITEFILE_ASYNC & 0xFF;

	return 0;
}
