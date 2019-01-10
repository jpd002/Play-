#include <assert.h>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include "../AppConfig.h"
#include "../PS2VM_Preferences.h"
#include "../Log.h"
#include "Iop_McServ.h"
#include "Iop_Sysmem.h"
#include "Iop_SifCmd.h"
#include "Iop_SifManPs2.h"
#include "IopBios.h"
#include "StdStreamUtils.h"
#include "MIPSAssembler.h"

using namespace Iop;
namespace filesystem = boost::filesystem;

#define CLUSTER_SIZE 0x400

#define LOG_NAME ("iop_mcserv")

#define MODULE_NAME "mcserv"
#define MODULE_VERSION 0x101

#define CUSTOM_STARTREADFAST 0x666
#define CUSTOM_PROCEEDREADFAST 0x667
#define CUSTOM_FINISHREADFAST 0x668

// clang-format off
const char* CMcServ::m_mcPathPreference[2] =
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
}

const char* CMcServ::GetMcPathPreference(unsigned int port)
{
	return m_mcPathPreference[port];
}

std::string CMcServ::GetId() const
{
	return MODULE_NAME;
}

std::string CMcServ::GetFunctionName(unsigned int) const
{
	return "unknown";
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
	switch(method)
	{
	case 0x01:
	case 0x78:
		GetInfo(args, argsSize, ret, retSize, ram);
		break;
	case 0x02:
	case 0x71:
		Open(args, argsSize, ret, retSize, ram);
		break;
	case 0x03:
	case 0x72:
		Close(args, argsSize, ret, retSize, ram);
		break;
	case 0x04:
		Seek(args, argsSize, ret, retSize, ram);
		break;
	case 0x05:
	case 0x73:
		Read(args, argsSize, ret, retSize, ram);
		break;
	case 0x06:
	case 0x74:
		Write(args, argsSize, ret, retSize, ram);
		break;
	case 0x0A:
	case 0x7A:
		Flush(args, argsSize, ret, retSize, ram);
		break;
	case 0x0C:
		ChDir(args, argsSize, ret, retSize, ram);
		break;
	case 0x0D:
	case 0x76: //Used by homebrew (ex.: ps2infones)
		GetDir(args, argsSize, ret, retSize, ram);
		break;
	case 0x0F:
	case 0x79:
		Delete(args, argsSize, ret, retSize, ram);
		break;
	case 0x15:
		GetSlotMax(args, argsSize, ret, retSize, ram);
		break;
	case 0x16:
		return ReadFast(args, argsSize, ret, retSize, ram);
		break;
	case 0xFE:
	case 0x70:
		//Get version?
		GetVersionInformation(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown RPC method invoked (0x%08X).\r\n", method);
		break;
	}
	return true;
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

	//Return values
	//  0 if same card as previous call
	//  -1 if new formatted card
	//  -2 if new unformatted card
	//> -2 on error
	ret[0] = 0;
}

void CMcServ::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);

	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Open(port = %i, slot = %i, flags = %i, name = %s);\r\n",
	                          cmd->port, cmd->slot, cmd->flags, cmd->name);

	if(cmd->port > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	filesystem::path filePath;

	try
	{
		filePath = GetAbsoluteFilePath(cmd->port, cmd->slot, cmd->name);
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
			filesystem::create_directory(filePath);
			result = 0;
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
			if(!boost::filesystem::exists(filePath))
			{
				//Create file if it doesn't exist
				Framework::CreateOutputStdStream(filePath.native());
			}
		}

		if(cmd->flags & OPEN_FLAG_TRUNC)
		{
			if(boost::filesystem::exists(filePath))
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

	file->Seek(cmd->offset, origin);
	ret[0] = static_cast<uint32>(file->Tell());
}

void CMcServ::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Read(handle = %i, size = 0x%08X, bufferAddress = 0x%08X, paramAddress = 0x%08X);\r\n",
	                          cmd->handle, cmd->size, cmd->bufferAddress, cmd->paramAddress);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	assert(cmd->bufferAddress != 0);
	void* dst = &ram[cmd->bufferAddress];

	if(cmd->paramAddress != 0)
	{
		//This param buffer is used in the callback after calling this method... No clue what it's for
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[0] = 0;
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[1] = 0;
	}

	ret[0] = static_cast<uint32>(file->Read(dst, cmd->size));
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

	CLog::GetInstance().Print(LOG_NAME, "ChDir(port = %i, slot = %i, tableAddress = 0x%08X, name = %s);\r\n",
	                          cmd->port, cmd->slot, cmd->tableAddress, cmd->name);

	uint32 result = -1;

	try
	{
		filesystem::path newCurrentDirectory;
		filesystem::path requestedDirectory(cmd->name);

		if(!requestedDirectory.root_directory().empty())
		{
			if(requestedDirectory.string() != "/")
			{
				newCurrentDirectory = requestedDirectory;
			}
			else
			{
				newCurrentDirectory.clear();
			}
		}
		else
		{
			newCurrentDirectory = m_currentDirectory / requestedDirectory;
		}

		auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[cmd->port]);
		mcPath /= newCurrentDirectory;

		if(filesystem::exists(mcPath) && filesystem::is_directory(mcPath))
		{
			m_currentDirectory = newCurrentDirectory;
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

	CLog::GetInstance().Print(LOG_NAME, "GetDir(port = %i, slot = %i, flags = %i, maxEntries = %i, tableAddress = 0x%08X, name = %s);\r\n",
	                          cmd->port, cmd->slot, cmd->flags, cmd->maxEntries, cmd->tableAddress, cmd->name);

	if(cmd->port > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	try
	{
		if(cmd->flags == 0)
		{
			m_pathFinder.Reset();

			auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[cmd->port]);
			if(cmd->name[0] != '/')
			{
				mcPath /= m_currentDirectory;
			}
			mcPath = filesystem::absolute(mcPath);

			if(!filesystem::exists(mcPath))
			{
				//Directory doesn't exist
				ret[0] = RET_NO_ENTRY;
				return;
			}

			filesystem::path searchPath = mcPath / cmd->name;
			searchPath.remove_filename();
			if(!filesystem::exists(searchPath))
			{
				//Specified directory doesn't exist, this is an error
				ret[0] = RET_NO_ENTRY;
				return;
			}

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

void CMcServ::Delete(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	auto cmd = reinterpret_cast<const CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Delete(port = %d, slot = %d, name = '%s');\r\n", cmd->port, cmd->slot, cmd->name);

	try
	{
		auto filePath = GetAbsoluteFilePath(cmd->port, cmd->slot, cmd->name);
		if(boost::filesystem::exists(filePath))
		{
			boost::filesystem::remove(filePath);
			ret[0] = 0;
		}
		else
		{
			ret[0] = RET_NO_ENTRY;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error while executing Delete: %s.\r\n", exception.what());
		ret[0] = -1;
		return;
	}
}

void CMcServ::GetSlotMax(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	int port = args[1];
	CLog::GetInstance().Print(LOG_NAME, "GetSlotMax(port = %i);\r\n", port);
	ret[0] = 1;
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

	ret[0] = 1;

	auto moduleData = reinterpret_cast<MODULEDATA*>(m_ram + m_moduleDataAddr);
	moduleData->readFastHandle = cmd->handle;
	moduleData->readFastSize = cmd->size;
	moduleData->readFastBufferAddress = cmd->bufferAddress;

	m_bios.TriggerCallback(m_readFastAddr);
	return false;
}

void CMcServ::GetVersionInformation(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x30);
	assert(retSize == 0x0C);

	ret[0] = 0x00000000;
	ret[1] = 0x0000020A; //mcserv version
	ret[2] = 0x0000020E; //mcman version

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");
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
	uint32 amountRead = file->Read(cluster, readSize);
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

boost::filesystem::path CMcServ::GetAbsoluteFilePath(unsigned int port, unsigned int slot, const char* name) const
{
	auto mcPath = CAppConfig::GetInstance().GetPreferencePath(m_mcPathPreference[port]);
	auto requestedFilePath = boost::filesystem::path(name);

	if(!requestedFilePath.root_directory().empty())
	{
		return mcPath / requestedFilePath;
	}
	else
	{
		return mcPath / m_currentDirectory / requestedFilePath;
	}
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

void CMcServ::CPathFinder::Search(const boost::filesystem::path& basePath, const char* filter)
{
	m_basePath = basePath;

	std::string filterPathString = filter;
	if(filterPathString[0] != '/')
	{
		filterPathString = "/" + filterPathString;
	}

	{
		std::string filterExpString = filterPathString;
		boost::replace_all(filterExpString, "\\", "\\\\");
		boost::replace_all(filterExpString, ".", "\\.");
		boost::replace_all(filterExpString, "?", ".?");
		boost::replace_all(filterExpString, "*", ".*");
		m_filterExp = std::regex(filterExpString);
	}

	auto filterPath = boost::filesystem::path(filterPathString);
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
		entry.attributes = 0x8427;
		m_entries.push_back(entry);
	}

	if(std::regex_match(parentDirPathString, m_filterExp))
	{
		ENTRY entry;
		memset(&entry, 0, sizeof(entry));
		strcpy(reinterpret_cast<char*>(entry.name), "..");
		entry.size = 0;
		entry.attributes = 0x8427;
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

void CMcServ::CPathFinder::SearchRecurse(const filesystem::path& path)
{
	bool found = false;
	filesystem::directory_iterator endIterator;

	for(filesystem::directory_iterator elementIterator(path);
	    elementIterator != endIterator; elementIterator++)
	{
		boost::filesystem::path relativePath(*elementIterator);
		std::string relativePathString(relativePath.generic_string());

		//"Extract" a more appropriate relative path from the memory card point of view
		relativePathString.erase(0, m_basePath.string().size());

		//Attempt to match this against the filter
		if(std::regex_match(relativePathString, m_filterExp))
		{
			//Fill in the information
			ENTRY entry;
			memset(&entry, 0, sizeof(entry));

			strncpy(reinterpret_cast<char*>(entry.name), relativePath.filename().string().c_str(), 0x1F);
			entry.name[0x1F] = 0;

			if(filesystem::is_directory(*elementIterator))
			{
				entry.size = 0;
				entry.attributes = 0x8427;
			}
			else
			{
				entry.size = static_cast<uint32>(filesystem::file_size(*elementIterator));
				entry.attributes = 0x8497;
			}

			//Fill in modification date info
			{
				auto changeDate = filesystem::last_write_time(*elementIterator);
				auto localChangeDate = localtime(&changeDate);

				entry.modificationTime.second = localChangeDate->tm_sec;
				entry.modificationTime.minute = localChangeDate->tm_min;
				entry.modificationTime.hour = localChangeDate->tm_hour;
				entry.modificationTime.day = localChangeDate->tm_mday;
				entry.modificationTime.month = localChangeDate->tm_mon;
				entry.modificationTime.year = localChangeDate->tm_year + 1900;
			}

			//boost::filesystem doesn't provide a way to get creation time, so just make it the same as modification date
			entry.creationTime = entry.modificationTime;

			m_entries.push_back(entry);
			found = true;
		}

		if(filesystem::is_directory(*elementIterator) && !found)
		{
			SearchRecurse(*elementIterator);
		}
	}
}
