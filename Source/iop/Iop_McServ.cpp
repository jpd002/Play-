#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <cstring>
#include <states/XmlStateFile.h>
#include <xml/Utils.h>
#include "string_format.h"
#include "../AppConfig.h"
#include "../PS2VM_Preferences.h"
#include "../Log.h"
#include "Iop_McServ.h"
#include "Iop_PathUtils.h"
#include "Iop_Sysmem.h"
#include "Iop_SifCmd.h"
#include "Iop_SifManPs2.h"
#include "IopBios.h"
#include "StdStreamUtils.h"
#include "StringUtils.h"
#include "MIPSAssembler.h"
#include "FilesystemUtils.h"

using namespace Iop;

#define CLUSTER_SIZE 0x400

#define LOG_NAME ("iop_mcserv")

#define MODULE_NAME "mcserv"
#define MODULE_VERSION 0x101

#define CUSTOM_STARTREADFAST 0x666
#define CUSTOM_PROCEEDREADFAST 0x667
#define CUSTOM_FINISHREADFAST 0x668

#define SEPARATOR_CHAR '/'

#define CMD_DELAY_DEFAULT 100000

#define STATE_MEMCARDS_FILE ("iop_mcserv/memcards.xml")
#define STATE_MEMCARDS_NODE "Memorycards"
#define STATE_MEMCARDS_CARDNODE "Memorycard"

#define STATE_MEMCARDS_CARDNODE_PORTATTRIBUTE ("Port")
#define STATE_MEMCARDS_CARDNODE_KNOWNATTRIBUTE ("Known")

#define MC_FILE_ATTR_FOLDER (MC_FILE_0400 | MC_FILE_ATTR_EXISTS | MC_FILE_ATTR_SUBDIR | MC_FILE_ATTR_READABLE | MC_FILE_ATTR_WRITEABLE | MC_FILE_ATTR_EXECUTABLE)

// clang-format off
const char* CMcServ::m_mcPathPreference[MAX_PORTS] =
{
	PREF_PS2_MC0_DIRECTORY,
	PREF_PS2_MC1_DIRECTORY,
};
// clang-format on

CMcServ::CMcServ(CIopBios& bios, CSifMan& sifMan, CSifCmd& sifCmd, CSysmem& sysMem, uint8* ram)
    : m_bios(bios)
    , m_sifMan(sifMan)
    , m_sifCmd(sifCmd)
    , m_sysMem(sysMem)
    , m_ram(ram)
{
	m_moduleDataAddr = m_sysMem.AllocateMemory(sizeof(MODULEDATA), 0, 0);
	sifMan.RegisterModule(MODULE_ID, this);
	BuildCustomCode();
	SetModuleVersion(1000);
}

void CMcServ::SetModuleVersion(unsigned int)
{
	//We don't really care about the version here.

	//SetModuleVersion is called when IOP is reset, make sure we also reset the state of this module.
	//Calling Init doesn't reset the state of known memory cards.
	//It's probably only reset when the module is reloaded.

	for(bool& knownMemoryCard : m_knownMemoryCards)
	{
		knownMemoryCard = false;
	}
}

const char* CMcServ::GetMcPathPreference(unsigned int port)
{
	return m_mcPathPreference[port];
}

std::string CMcServ::EncodeMcName(const std::string& inputName)
{
	std::string result;
	for(size_t i = 0; i < inputName.size(); i++)
	{
		auto inputChar = inputName[i];
		if(inputChar == 0) break;
		//':' is not allowed on Windows
		if(inputChar == ':')
		{
			result += string_format("%%%02X", inputChar);
		}
		else
		{
			result += inputChar;
		}
	}
	return result;
}

std::string CMcServ::DecodeMcName(const std::string& inputName)
{
	std::string result;
	for(size_t i = 0; i < inputName.size(); i++)
	{
		auto inputChar = inputName[i];
		if(inputChar == '%')
		{
			int decodedChar = 0;
			FRAMEWORK_MAYBE_UNUSED int scanCount = sscanf(inputName.c_str() + i, "%%%02X", &decodedChar);
			assert(scanCount == 1);
			result += decodedChar;
			i += 2;
		}
		else
		{
			result += inputChar;
		}
	}
	return result;
}

std::string CMcServ::MakeAbsolutePath(const std::string& inputPath)
{
	auto frags = StringUtils::Split(inputPath, '/', true);
	std::vector<std::string> newFrags;
	for(const auto& frag : frags)
	{
		if(frag.empty()) continue;
		if(frag == ".")
		{
			continue;
		}
		if(frag == "..")
		{
			if(newFrags.empty())
			{
				//Going up too much, don't bother
				continue;
			}
			newFrags.pop_back();
		}
		else
		{
			newFrags.push_back(frag);
		}
	}
	auto outputPath = std::string();
	for(const auto& frag : newFrags)
	{
		if(frag.empty()) continue;
		outputPath += "/";
		outputPath += frag;
	}
	return outputPath;
}

std::string CMcServ::GetId() const
{
	return MODULE_NAME;
}

std::string CMcServ::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CMcServ::CountTicks(uint32 ticks, CSifMan* sifMan)
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	if(moduleData->pendingCommand == CMD_ID_NONE) return;

	moduleData->pendingCommandDelay -= std::min<uint32>(moduleData->pendingCommandDelay, ticks);
	if(moduleData->pendingCommandDelay == 0)
	{
		sifMan->SendCallReply(MODULE_ID, nullptr);
		moduleData->pendingCommand = CMD_ID_NONE;
	}
}

void CMcServ::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case CUSTOM_STARTREADFAST:
		StartReadFast(context);
		break;
	case CUSTOM_PROCEEDREADFAST:
		ProceedReadFast(context);
		break;
	case CUSTOM_FINISHREADFAST:
		FinishReadFast(context);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown module method invoked (%d).\r\n", functionId);
		break;
	}
}

bool CMcServ::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	bool isDirectCall = (method & CMD_FLAG_DIRECT) != 0;
	method &= ~CMD_FLAG_MASK;

	switch(method)
	{
	case CMD_ID_GETINFO:
	case 0x78:
		// Many games seem to be sensitive to the delay response of this function:
		//- Nights Into Dreams (issues 2 Syncs very close to each other, infinite loop if GetInfo is instantenous)
		//- Melty Blood Actress Again
		//- Baroque
		//- Naruto Shippuden: Ultimate Ninja 5 (if GetInfo doesn't return quickly enough, MC thread is killed and game will hang)
		GetInfo(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_OPEN:
	case 0x71:
		// Operation Winback 2 expects a delay here, otherwise it hangs trying to read or write a save file
		Open(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_CLOSE:
	case 0x72:
		Close(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_SEEK:
		Seek(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_READ:
	case 0x73:
		// Operation Winback 2 expects a delay here, otherwise it hangs trying to read a save file
		Read(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_WRITE:
	case 0x74:
		// Operation Winback 2 expects a delay here, otherwise it hangs trying to write a save file
		Write(args, argsSize, ret, retSize, ram);
		break;
	case 0x0A:
	case 0x7A:
		Flush(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_CHDIR:
		ChDir(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_GETDIR:
	case 0x76: //Used by homebrew (ex.: ps2infones)
		GetDir(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_SETFILEINFO:
	case 0x7C:
		SetFileInfo(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_DELETE:
	case 0x79:
		Delete(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_GETENTSPACE:
		GetEntSpace(args, argsSize, ret, retSize, ram);
		break;
	case CMD_ID_SETTHREADPRIORITY:
		SetThreadPriority(args, argsSize, ret, retSize, ram);
		break;
	case 0x15:
		GetSlotMax(args, argsSize, ret, retSize, ram);
		break;
	case 0x16:
		return ReadFast(args, argsSize, ret, retSize, ram);
		break;
	case 0x1B:
		WriteFast(args, argsSize, ret, retSize, ram);
		break;
	case 0xFE:
	case 0x70:
		Init(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown RPC method invoked (0x%08X).\r\n", method);
		return true;
	}

	if(!isDirectCall)
	{
		// Delay all commands a bit (except ReadFast, which has a different mecanism)
		// Direct calls also don't require SIF RPC replies
		// Fixes games which receive the rpc response before they are ready to receive them
		auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
		assert(moduleData->pendingCommand == CMD_ID_NONE);
		moduleData->pendingCommand = method;
		moduleData->pendingCommandDelay = CMD_DELAY_DEFAULT;
	}

	return false;
}

void CMcServ::BuildCustomCode()
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);

	auto exportTable = reinterpret_cast<uint32*>(moduleData->trampoline);
	*(exportTable++) = 0x41E00000;
	*(exportTable++) = 0;
	*(exportTable++) = MODULE_VERSION;
	strcpy(reinterpret_cast<char*>(exportTable), MODULE_NAME);
	exportTable += (strlen(MODULE_NAME) + 3) / 4;

	{
		CMIPSAssembler assembler(exportTable);
		uint32 codeBase = (reinterpret_cast<uint8*>(exportTable) - m_ram);

		m_startReadFastAddr = codeBase + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_STARTREADFAST);

		m_proceedReadFastAddr = codeBase + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_PROCEEDREADFAST);

		m_finishReadFastAddr = codeBase + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_FINISHREADFAST);

		m_readFastAddr = codeBase + AssembleReadFast(assembler);

		exportTable += assembler.GetProgramSize();
	}

	assert((reinterpret_cast<uint8*>(exportTable) - moduleData->trampoline) <= MODULEDATA::TRAMPOLINE_SIZE);
}

uint32 CMcServ::AssembleReadFast(CMIPSAssembler& assembler)
{
	//Extra stack alloc for SifCallRpc
	static const int16 stackAlloc = 0x100;

	uint32 result = assembler.GetProgramSize() * 4;
	auto readNextLabel = assembler.CreateLabel();

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
	assembler.SW(CMIPS::RA, 0xFC, CMIPS::SP);
	assembler.SW(CMIPS::S0, 0xF8, CMIPS::SP);

	assembler.LI(CMIPS::S0, m_moduleDataAddr);

	assembler.JAL(m_startReadFastAddr);
	assembler.NOP();

	assembler.MarkLabel(readNextLabel);

	assembler.JAL(m_proceedReadFastAddr);
	assembler.NOP();

	assembler.LW(CMIPS::A0, offsetof(MODULEDATA, readFastSize), CMIPS::S0);
	assembler.BNE(CMIPS::A0, CMIPS::R0, readNextLabel);
	assembler.NOP();

	assembler.JAL(m_finishReadFastAddr);
	assembler.NOP();

	assembler.LW(CMIPS::S0, 0xF8, CMIPS::SP);
	assembler.LW(CMIPS::RA, 0xFC, CMIPS::SP);
	assembler.JR(CMIPS::RA);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);

	return result;
}

void CMcServ::GetInfo(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x1C);

	//The layout of this can actually vary according to the version of the
	//MCSERV module currently loaded
	uint32 port = args[1];
	uint32 slot = args[2];
	bool wantFormatted = args[3] != 0;
	bool wantFreeSpace = args[4] != 0;
	bool wantType = args[5] != 0;
	uint32* retBuffer = reinterpret_cast<uint32*>(&ram[args[7]]);

	CLog::GetInstance().Print(LOG_NAME, "GetInfo(port = %i, slot = %i, wantType = %i, wantFreeSpace = %i, wantFormatted = %i, retBuffer = 0x%08X);\r\n",
	                          port, slot, wantType, wantFreeSpace, wantFormatted, args[7]);

	if(HandleInvalidPortOrSlot(port, slot, ret))
	{
		return;
	}

	if(wantType)
	{
		retBuffer[0x00] = 2; //2 -> PS2 memory card
	}
	if(wantFreeSpace)
	{
		retBuffer[0x01] = 0x2000; //Number of clusters, cluster size = 1024 bytes
	}
	if(wantFormatted)
	{
		retBuffer[0x24] = 1;
	}

	if(port >= MAX_PORTS)
	{
		assert(0);
		ret[0] = -2;
		return;
	}

	bool isKnownCard = m_knownMemoryCards[port];
	m_knownMemoryCards[port] = true;

	//Return values
	//  0 if same card as previous call
	//  -1 if new formatted card
	//  -2 if new unformatted card
	//> -2 on error
	ret[0] = isKnownCard ? 0 : -1;
}

void CMcServ::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);

	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Open(port = %i, slot = %i, flags = %i, name = '%s');\r\n",
	                          cmd->port, cmd->slot, cmd->flags, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	auto name = EncodeMcName(cmd->name);

	fs::path filePath;

	try
	{
		filePath = GetHostFilePath(cmd->port, cmd->slot, name.c_str());
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing Open: %s.\r\n", exception.what());
		ret[0] = -1;
		return;
	}

	if(cmd->flags == 0x40)
	{
		//Directory only?
		uint32 result = -1;
		try
		{
			if(fs::exists(filePath))
			{
				result = RET_NO_ENTRY;
			}
			else
			{
				fs::create_directory(filePath);
				result = 0;
			}
		}
		catch(...)
		{
		}
		ret[0] = result;
		return;
	}
	else
	{
		if(cmd->flags & OPEN_FLAG_CREAT)
		{
			if(!fs::exists(filePath))
			{
				//Create file if it doesn't exist
				try
				{
					Framework::CreateOutputStdStream(filePath.native());
				}
				catch(...)
				{
					//Might fail in some conditions (ex.: if file is to be created in a directory that doesn't exist).
					ret[0] = RET_NO_ENTRY;
					return;
				}
			}
		}

		if(cmd->flags & OPEN_FLAG_TRUNC)
		{
			if(fs::exists(filePath))
			{
				//Create file (discard contents) if it exists
				Framework::CreateOutputStdStream(filePath.native());
			}
		}

		//At this point, we assume that the file has been created or truncated
		try
		{
			auto file = Framework::CreateUpdateExistingStdStream(filePath.native());
			uint32 handle = GenerateHandle();
			if(handle == -1)
			{
				//Exhausted all file handles
				throw std::exception();
			}
			m_files[handle] = std::move(file);
			ret[0] = handle;
		}
		catch(...)
		{
			//Not existing file?
			ret[0] = RET_NO_ENTRY;
			return;
		}
	}
}

void CMcServ::Close(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Close(handle = %i);\r\n", cmd->handle);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	file->Clear();

	ret[0] = 0;
}

void CMcServ::Seek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Seek(handle = %i, offset = 0x%08X, origin = 0x%08X);\r\n",
	                          cmd->handle, cmd->offset, cmd->origin);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	Framework::STREAM_SEEK_DIRECTION origin = Framework::STREAM_SEEK_SET;
	switch(cmd->origin)
	{
	case 0:
		origin = Framework::STREAM_SEEK_SET;
		break;
	case 1:
		origin = Framework::STREAM_SEEK_CUR;
		break;
	case 2:
		origin = Framework::STREAM_SEEK_END;
		break;
	default:
		assert(0);
		break;
	}

	file->Seek(static_cast<int32>(cmd->offset), origin);
	ret[0] = static_cast<uint32>(file->Tell());
}

void CMcServ::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Read(handle = %i, size = 0x%08X, bufferAddress = 0x%08X, paramAddress = 0x%08X);\r\n",
	                          cmd->handle, cmd->size, cmd->bufferAddress, cmd->paramAddress);

	if(cmd->paramAddress != 0)
	{
		//This param buffer is used in the callback after calling this method.
		//It seems to be used to ferry some data to reduce the number of transfers between IOP and EE.
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[0] = 0;
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[1] = 0;
	}

	auto file = GetFileFromHandle(cmd->handle);
	if(!file)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Warning. Attempted to read from an invalid fd (%d).\r\n", cmd->handle);
		ret[0] = RET_PERMISSION_DENIED;
		return;
	}

	assert(cmd->bufferAddress != 0);
	void* dst = &ram[cmd->bufferAddress];

	if(file->IsEOF())
	{
		ret[0] = 0;
	}
	else
	{
		ret[0] = static_cast<uint32>(file->Read(dst, cmd->size));

		//Sync pointer for games that write after read without seeking or flushing
		file->Seek(file->Tell(), Framework::STREAM_SEEK_SET);
	}
}

void CMcServ::Write(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Write(handle = %i, nSize = 0x%08X, bufferAddress = 0x%08X, origin = 0x%08X);\r\n",
	                          cmd->handle, cmd->size, cmd->bufferAddress, cmd->origin);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = RET_PERMISSION_DENIED;
		assert(0);
		return;
	}

	const void* dst = &ram[cmd->bufferAddress];
	uint32 result = 0;

	//Write "origin" bytes from "data" field first
	if(cmd->origin != 0)
	{
		file->Write(cmd->data, cmd->origin);
		result += cmd->origin;
	}

	result += static_cast<uint32>(file->Write(dst, cmd->size));
	ret[0] = result;

	//Force flushing for games that read after write without seeking or flushing
	file->Flush();
}

void CMcServ::Flush(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Flush(handle = %d);\r\n", cmd->handle);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	file->Flush();

	ret[0] = 0;
}

void CMcServ::ChDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);
	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "ChDir(port = %i, slot = %i, tableAddress = 0x%08X, name = '%s');\r\n",
	                          cmd->port, cmd->slot, cmd->tableAddress, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	uint32 result = -1;
	auto& currentDirectory = m_currentDirectory[cmd->port];

	//Write out current directory
	if(cmd->tableAddress != 0)
	{
		//Make sure we return '/' even if the current directory is empty, needed by Silent Hill 3
		auto curDir = currentDirectory.empty() ? std::string(1, SEPARATOR_CHAR) : currentDirectory;

		const size_t maxCurDirSize = 256;
		char* currentDirOut = reinterpret_cast<char*>(ram + cmd->tableAddress);
		strncpy(currentDirOut, curDir.c_str(), maxCurDirSize - 1);
	}

	try
	{
		std::string newCurrentDirectory;
		std::string requestedDirectory = EncodeMcName(cmd->name);

		if(!requestedDirectory.empty() && (requestedDirectory[0] == SEPARATOR_CHAR))
		{
			if(requestedDirectory.length() != 1)
			{
				newCurrentDirectory = requestedDirectory;
			}
			else
			{
				//Clear if only separator char
				newCurrentDirectory.clear();
			}
		}
		else
		{
			newCurrentDirectory = currentDirectory + SEPARATOR_CHAR + requestedDirectory;
		}

		//Some games (EA games) will try to ChDir('..') from the MC's root
		//Kim Possible: What's the Switch will also attempt this and rely on the result
		//to consider other MC operations to be successes.

		newCurrentDirectory = MakeAbsolutePath(newCurrentDirectory);

		auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[cmd->port]);
		auto hostPath = Iop::PathUtils::MakeHostPath(mcPath, newCurrentDirectory.c_str());

		if(!Iop::PathUtils::IsInsideBasePath(mcPath, hostPath))
		{
			//This shouldn't happen, but fail just in case.
			assert(false);
			result = RET_NO_ENTRY;
		}
		else if(fs::exists(hostPath) && fs::is_directory(hostPath))
		{
			currentDirectory = newCurrentDirectory;
			result = 0;
		}
		else
		{
			//Not found (I guess)
			result = RET_NO_ENTRY;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing ChDir: %s.\r\n", exception.what());
	}

	ret[0] = result;
}

void CMcServ::GetDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 result = 0;

	assert(argsSize >= 0x414);

	auto cmd = reinterpret_cast<const CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "GetDir(port = %i, slot = %i, flags = %i, maxEntries = %i, tableAddress = 0x%08X, name = '%s');\r\n",
	                          cmd->port, cmd->slot, cmd->flags, cmd->maxEntries, cmd->tableAddress, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	try
	{
		if(cmd->flags == 0)
		{
			m_pathFinder.Reset();

			auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[cmd->port]);
			if(cmd->name[0] != SEPARATOR_CHAR)
			{
				mcPath = Iop::PathUtils::MakeHostPath(mcPath, m_currentDirectory[cmd->port].c_str());
			}
			mcPath = fs::absolute(mcPath);

			if(!fs::exists(mcPath))
			{
				//Directory doesn't exist
				ret[0] = RET_NO_ENTRY;
				return;
			}

			auto searchPath = Iop::PathUtils::MakeHostPath(mcPath, cmd->name);
			searchPath.remove_filename();
			if(!fs::exists(searchPath))
			{
				//Specified directory doesn't exist, this is an error
				ret[0] = RET_NO_ENTRY;
				return;
			}

			assert(*mcPath.string().rbegin() != '/');
			m_pathFinder.Search(mcPath, cmd->name);
		}

		auto entries = (cmd->maxEntries > 0) ? reinterpret_cast<ENTRY*>(&ram[cmd->tableAddress]) : nullptr;
		result = m_pathFinder.Read(entries, cmd->maxEntries);
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing GetDir: %s.\r\n", exception.what());
	}

	ret[0] = result;
}

void CMcServ::SetFileInfo(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<const CMD*>(args);
	CLog::GetInstance().Print(LOG_NAME, "SetFileInfo(port = %i, slot = %i, flags = %i, name = '%s');\r\n", cmd->port, cmd->slot, cmd->flags, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	auto entry = reinterpret_cast<ENTRY*>(ram + cmd->tableAddress);

	auto flags = cmd->flags;

	if(flags & MC_FILE_ATTR_FILE)
	{
		auto filePath1 = GetHostFilePath(cmd->port, cmd->slot, cmd->name);
		auto filePath2 = GetHostFilePath(cmd->port, cmd->slot, cmd->name);
		filePath2.replace_filename(reinterpret_cast<const char*>(entry->name));

		if(filePath1 != filePath2)
		{
			try
			{
				if(!fs::exists(filePath1))
				{
					ret[0] = RET_NO_ENTRY;
					return;
				}

				fs::rename(filePath1, filePath2);
			}
			catch(...)
			{
				ret[0] = -1;
				return;
			}
		}
	}

	flags &= ~MC_FILE_ATTR_FILE;

	if(flags != 0)
	{
		// TODO: We only support file renaming for the moment
		CLog::GetInstance().Warn(LOG_NAME, "Setting unknown file attribute flag %i\r\n", cmd->flags);
	}

	ret[0] = 0;
}

void CMcServ::Delete(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<const CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Delete(port = %d, slot = %d, name = '%s');\r\n", cmd->port, cmd->slot, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	try
	{
		auto filePath = GetHostFilePath(cmd->port, cmd->slot, cmd->name);
		if(fs::exists(filePath))
		{
			fs::remove(filePath);
			ret[0] = 0;
		}
		else
		{
			ret[0] = RET_NO_ENTRY;
		}
	}
	catch(const fs::filesystem_error& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing Delete: %s.\r\n", exception.what());
		auto errorCode = exception.code();
		if(errorCode == std::errc::directory_not_empty)
		{
			//Musashi Samurai Legends will try to delete a directory when overwriting a save
			ret[0] = RET_NOT_EMPTY;
		}
		else
		{
			ret[0] = -1;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing Delete: %s.\r\n", exception.what());
		ret[0] = -1;
	}
}

void CMcServ::GetEntSpace(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "GetEntSpace(port = %i, slot = %i, flags = %i, name = '%s');\r\n",
	                          cmd->port, cmd->slot, cmd->flags, cmd->name);

	if(HandleInvalidPortOrSlot(cmd->port, cmd->slot, ret))
	{
		return;
	}

	auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[cmd->port]);
	auto savePath = Iop::PathUtils::MakeHostPath(mcPath, cmd->name);

	try
	{
		if(fs::exists(savePath) && fs::is_directory(savePath))
		{
			// Arbitrarity number, allows Drakengard to detect MC
			ret[0] = 0xFE;
		}
		else
		{
			ret[0] = RET_NO_ENTRY;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing GetEntSpace: %s.\r\n", exception.what());
		ret[0] = -1;
	}
}

void CMcServ::SetThreadPriority(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<CMD*>(args);
	uint32 priority = *reinterpret_cast<uint32*>(cmd->name);

	CLog::GetInstance().Print(LOG_NAME, "SetThreadPriority(priority = %d);\r\n", priority);

	//We don't really care about thread priority here, just report a success
	ret[0] = 0;
}

void CMcServ::GetSlotMax(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	int port = args[1];
	CLog::GetInstance().Print(LOG_NAME, "GetSlotMax(port = %i);\r\n", port);
	ret[0] = MAX_SLOTS;
}

bool CMcServ::ReadFast(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//Based on mcemu code: https://github.com/ifcaro/Open-PS2-Loader/blob/master/modules/mcemu/mcemu_rpc.c

	auto cmd = reinterpret_cast<FILECMD*>(args);
	CLog::GetInstance().Print(LOG_NAME, "ReadFast(handle = %d, size = 0x%08X, bufferAddress = 0x%08X, paramAddress = 0x%08X);\r\n",
	                          cmd->handle, cmd->size, cmd->bufferAddress, cmd->paramAddress);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		return true;
	}

	//Returns the amount of bytes read
	assert(cmd->size >= file->GetRemainingLength());
	ret[0] = cmd->size;

	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	moduleData->readFastHandle = cmd->handle;
	moduleData->readFastSize = cmd->size;
	moduleData->readFastBufferAddress = cmd->bufferAddress;

	m_bios.TriggerCallback(m_readFastAddr);
	return false;
}

void CMcServ::WriteFast(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "WriteFast(handle = %d, size = 0x%08X, bufferAddress = 0x%08X, paramAddress = 0x%08X);\r\n",
	                          cmd->handle, cmd->size, cmd->bufferAddress, cmd->paramAddress);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = RET_PERMISSION_DENIED;
		return;
	}

	const void* dst = &ram[cmd->bufferAddress];
	uint32 result = 0;

	result += static_cast<uint32>(file->Write(dst, cmd->size));
	ret[0] = result;
}

void CMcServ::Init(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x30);
	assert(retSize == 0x0C);

	ret[0] = 0x00000000;
	ret[1] = 0x0000020A; //mcserv version
	ret[2] = 0x0000020E; //mcman version

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");
}

bool CMcServ::HandleInvalidPortOrSlot(uint32 port, uint32 slot, uint32* ret)
{
	if(port >= MAX_PORTS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Called mc function with invalid port %d\r\n", port);
		ret[0] = -1;
		return true;
	}

	if(slot >= MAX_SLOTS)
	{
		//Just warn if an invalid slot is specified. Should not be a big deal since we never use that parameter
		//in our functions. It shouldn't also matter since we don't support multitap.
		//Note that some games specify 'slot = 1' while never checking GetSlotMax:
		//- Dragon Quest 8
		CLog::GetInstance().Warn(LOG_NAME, "Called mc function with invalid slot %d\r\n", slot);
	}

	return false;
}

void CMcServ::StartReadFast(CMIPS& context)
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	if(!moduleData->initialized)
	{
		context.m_State.nGPR[CMIPS::A0].nV0 = m_moduleDataAddr + offsetof(MODULEDATA, rpcClientData);
		context.m_State.nGPR[CMIPS::A1].nV0 = MODULE_ID;
		context.m_State.nGPR[CMIPS::A2].nV0 = 0; //Wait mode
		m_sifCmd.SifBindRpc(context);

		moduleData->initialized = true;
	}
}

void CMcServ::ProceedReadFast(CMIPS& context)
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);

	auto file = GetFileFromHandle(moduleData->readFastHandle);
	assert(file);

	uint32 readSize = std::min<uint32>(moduleData->readFastSize, CLUSTER_SIZE);

	uint8 cluster[CLUSTER_SIZE];
	FRAMEWORK_MAYBE_UNUSED uint32 amountRead = file->Read(cluster, readSize);
	assert(amountRead == readSize);
	moduleData->readFastSize -= readSize;

	if(auto sifManPs2 = dynamic_cast<CSifManPs2*>(&m_sifMan))
	{
		auto eeRam = sifManPs2->GetEeRam();
		memcpy(eeRam + moduleData->readFastBufferAddress, cluster, readSize);
	}

	reinterpret_cast<uint32*>(moduleData->rpcBuffer)[3] = readSize;

	context.m_State.nGPR[CMIPS::A0].nV0 = m_moduleDataAddr + offsetof(MODULEDATA, rpcClientData);
	context.m_State.nGPR[CMIPS::A1].nV0 = 2;
	context.m_State.nGPR[CMIPS::A2].nV0 = 0;
	context.m_State.nGPR[CMIPS::A3].nV0 = m_moduleDataAddr + offsetof(MODULEDATA, rpcBuffer);
	context.m_pMemoryMap->SetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10, MODULEDATA::RPC_BUFFER_SIZE);
	context.m_pMemoryMap->SetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x14, m_moduleDataAddr + offsetof(MODULEDATA, rpcBuffer));
	context.m_pMemoryMap->SetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x18, MODULEDATA::RPC_BUFFER_SIZE);
	context.m_pMemoryMap->SetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x1C, 0);
	context.m_pMemoryMap->SetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x20, 0);

	m_sifCmd.SifCallRpc(context);
}

void CMcServ::FinishReadFast(CMIPS& context)
{
	m_sifMan.SendCallReply(MODULE_ID, nullptr);
}

uint32 CMcServ::GenerateHandle()
{
	for(unsigned int i = 0; i < MAX_FILES; i++)
	{
		if(m_files[i].IsEmpty()) return i;
	}
	return -1;
}

Framework::CStdStream* CMcServ::GetFileFromHandle(uint32 handle)
{
	assert(handle < MAX_FILES);
	if(handle >= MAX_FILES)
	{
		return nullptr;
	}
	auto& file = m_files[handle];
	if(file.IsEmpty())
	{
		return nullptr;
	}
	return &file;
}

fs::path CMcServ::GetHostFilePath(unsigned int port, unsigned int slot, const char* path) const
{
	auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[port]);

	auto nameLength = strlen(path);
	if(nameLength == 0) return mcPath;

	std::string guestPath;
	if(path[0] == SEPARATOR_CHAR)
	{
		guestPath = path;
	}
	else
	{
		const auto& currentDirectory = m_currentDirectory[port];
		guestPath = currentDirectory + SEPARATOR_CHAR + std::string(path);
	}

	guestPath = MakeAbsolutePath(guestPath);

	return Iop::PathUtils::MakeHostPath(mcPath, guestPath.c_str());
}

void CMcServ::LoadState(Framework::CZipArchiveReader& archive)
{
	auto stateFile = CXmlStateFile(*archive.BeginReadFile(STATE_MEMCARDS_FILE));
	auto stateNode = stateFile.GetRoot();

	auto cardNodes = stateNode->SelectNodes(STATE_MEMCARDS_NODE "/" STATE_MEMCARDS_CARDNODE);

	int i = 0;
	for(auto fileNode : cardNodes)
	{
		Framework::Xml::GetAttributeIntValue(fileNode, STATE_MEMCARDS_CARDNODE_PORTATTRIBUTE, &i);
		Framework::Xml::GetAttributeBoolValue(fileNode, STATE_MEMCARDS_CARDNODE_KNOWNATTRIBUTE, &m_knownMemoryCards[i]);
	}
}

void CMcServ::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto stateFile = std::make_unique<CXmlStateFile>(STATE_MEMCARDS_FILE, STATE_MEMCARDS_NODE);
	auto stateNode = stateFile->GetRoot();

	for(unsigned int i = 0; i < MAX_PORTS; i++)
	{
		auto cardNode = std::make_unique<Framework::Xml::CNode>(STATE_MEMCARDS_CARDNODE, true);
		cardNode->InsertAttribute(Framework::Xml::CreateAttributeIntValue(STATE_MEMCARDS_CARDNODE_PORTATTRIBUTE, i));
		cardNode->InsertAttribute(Framework::Xml::CreateAttributeBoolValue(STATE_MEMCARDS_CARDNODE_KNOWNATTRIBUTE, m_knownMemoryCards[i]));
		stateNode->InsertNode(std::move(cardNode));
	}

	archive.InsertFile(std::move(stateFile));
}

/////////////////////////////////////////////
//CPathFinder Implementation
/////////////////////////////////////////////

CMcServ::CPathFinder::CPathFinder()
    : m_index(0)
{
}

CMcServ::CPathFinder::~CPathFinder()
{
}

void CMcServ::CPathFinder::Reset()
{
	m_entries.clear();
	m_index = 0;
}

void CMcServ::CPathFinder::Search(const fs::path& basePath, const char* filter)
{
	m_basePath = basePath;

	std::string filterPathString = filter;

	//Resolve relative paths (only when filter starts with one)
	if(filterPathString.find("./") == 0)
	{
		filterPathString = filterPathString.substr(1);
	}

	if(filterPathString[0] != '/')
	{
		filterPathString = "/" + filterPathString;
	}

	//Remove slash at end
	if((filterPathString.size() > 1) && (*filterPathString.rbegin() == '/'))
	{
		filterPathString.erase(filterPathString.end() - 1);
	}

	{
		std::string filterExpString = filterPathString;
		filterExpString = StringUtils::ReplaceAll(filterExpString, "\\", "\\\\");
		filterExpString = StringUtils::ReplaceAll(filterExpString, ".", "\\.");
		filterExpString = StringUtils::ReplaceAll(filterExpString, "?", ".?");
		filterExpString = StringUtils::ReplaceAll(filterExpString, "*", ".*");
		m_filterExp = std::regex(filterExpString);
	}

	auto filterPath = fs::path(filterPathString);
	filterPath.remove_filename();

	auto currentDirPath = filterPath / ".";
	auto parentDirPath = filterPath / "..";
	auto currentDirPathString = currentDirPath.generic_string();
	auto parentDirPathString = parentDirPath.generic_string();

	if(std::regex_match(currentDirPathString, m_filterExp))
	{
		ENTRY entry;
		memset(&entry, 0, sizeof(entry));
		strcpy(reinterpret_cast<char*>(entry.name), ".");
		entry.size = 0;
		entry.attributes = MC_FILE_ATTR_FOLDER;
		m_entries.push_back(entry);
	}

	if(std::regex_match(parentDirPathString, m_filterExp))
	{
		ENTRY entry;
		memset(&entry, 0, sizeof(entry));
		strcpy(reinterpret_cast<char*>(entry.name), "..");
		entry.size = 0;
		entry.attributes = MC_FILE_ATTR_FOLDER;
		m_entries.push_back(entry);
	}

	SearchRecurse(m_basePath);
}

unsigned int CMcServ::CPathFinder::Read(ENTRY* entry, unsigned int size)
{
	assert(m_index <= m_entries.size());
	unsigned int remaining = m_entries.size() - m_index;
	unsigned int readCount = std::min<unsigned int>(remaining, size);
	if(entry != nullptr)
	{
		for(unsigned int i = 0; i < readCount; i++)
		{
			entry[i] = m_entries[i + m_index];
		}
	}
	m_index += readCount;
	return readCount;
}

void CMcServ::CPathFinder::SearchRecurse(const fs::path& path)
{
	bool found = false;
	for(const auto& element : fs::directory_iterator(path))
	{
		const auto& relativePath(element.path());
		std::string relativePathString(relativePath.generic_string());

		//"Extract" a more appropriate relative path from the memory card point of view
		relativePathString.erase(0, m_basePath.generic_string().size());

		//Attempt to match this against the filter
		if(std::regex_match(relativePathString, m_filterExp))
		{
			//Fill in the information
			ENTRY entry;
			memset(&entry, 0, sizeof(entry));

			auto entryName = DecodeMcName(relativePath.filename().string());
			strncpy(reinterpret_cast<char*>(entry.name), entryName.c_str(), 0x1F);
			entry.name[0x1F] = 0;

			if(element.is_directory())
			{
				entry.size = CountEntries(element);
				entry.attributes = MC_FILE_ATTR_FOLDER;
			}
			else
			{
				entry.size = static_cast<uint32>(element.file_size());
				entry.attributes = MC_FILE_0400 | MC_FILE_ATTR_EXISTS | MC_FILE_ATTR_CLOSED | MC_FILE_ATTR_FILE | MC_FILE_ATTR_READABLE | MC_FILE_ATTR_WRITEABLE | MC_FILE_ATTR_EXECUTABLE;
			}

			//Fill in modification date info
			{
				auto changeSystemTime = Framework::ConvertFsTimeToSystemTime(element.last_write_time());
				auto localChangeDate = std::localtime(&changeSystemTime);

				entry.modificationTime.second = localChangeDate->tm_sec;
				entry.modificationTime.minute = localChangeDate->tm_min;
				entry.modificationTime.hour = localChangeDate->tm_hour;
				entry.modificationTime.day = localChangeDate->tm_mday;
				entry.modificationTime.month = localChangeDate->tm_mon;
				entry.modificationTime.year = localChangeDate->tm_year + 1900;
			}

			//std::filesystem doesn't provide a way to get creation time, so just make it the same as modification date
			entry.creationTime = entry.modificationTime;

			m_entries.push_back(entry);
			found = true;
		}

		if(element.is_directory() && !found)
		{
			SearchRecurse(element);
		}
	}
}

uint32 CMcServ::CPathFinder::CountEntries(const fs::path& path)
{
	uint32 entryCount = 0;
	assert(fs::is_directory(path));
	for(FRAMEWORK_MAYBE_UNUSED const auto& entry : fs::directory_iterator(path))
	{
		entryCount++;
	}
	return entryCount;
}
