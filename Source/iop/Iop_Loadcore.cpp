#include <cstring>
#include "Iop_Loadcore.h"
#include "Iop_Dynamic.h"
#include "IopBios.h"
#include "../Log.h"
#include "../states/RegisterStateFile.h"

using namespace Iop;

#define LOG_NAME "iop_loadcore"

#define STATE_VERSION_XML ("iop_loadcore/version.xml")
#define STATE_VERSION_MODULEVERSION ("moduleVersion")

#define FUNCTION_GETLIBRARYENTRYTABLE "GetLibraryEntryTable"
#define FUNCTION_FLUSHDCACHE "FlushDcache"
#define FUNCTION_REGISTERLIBRARYENTRIES "RegisterLibraryEntries"
#define FUNCTION_RELEASELIBRARYENTRIES "ReleaseLibraryEntries"
#define FUNCTION_QUERYBOOTMODE "QueryBootMode"
#define FUNCTION_SETREBOOTTIMELIBHANDLINGMODE "SetRebootTimeLibraryHandlingMode"

#define PATH_MAX_SIZE 252
#define ARGS_MAX_SIZE 252

CLoadcore::CLoadcore(CIopBios& bios, uint8* ram, CSifMan& sifMan)
    : m_bios(bios)
    , m_ram(ram)
{
	sifMan.RegisterModule(MODULE_ID, this);
}

void CLoadcore::SetModuleVersion(unsigned int moduleVersion)
{
	m_moduleVersion = moduleVersion;
}

std::string CLoadcore::GetId() const
{
	return "loadcore";
}

std::string CLoadcore::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 3:
		return FUNCTION_GETLIBRARYENTRYTABLE;
		break;
	case 5:
		return FUNCTION_FLUSHDCACHE;
		break;
	case 6:
		return FUNCTION_REGISTERLIBRARYENTRIES;
		break;
	case 7:
		return FUNCTION_RELEASELIBRARYENTRIES;
		break;
	case 12:
		return FUNCTION_QUERYBOOTMODE;
		break;
	case 27:
		return FUNCTION_SETREBOOTTIMELIBHANDLINGMODE;
		break;
	default:
		return "unknown";
		break;
	}
}

void CLoadcore::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 3:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetLibraryEntryTable());
		break;
	case 5:
		//FlushDCache
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(RegisterLibraryEntries(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = ReleaseLibraryEntries(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 12:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(QueryBootMode(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 27:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SetRebootTimeLibraryHandlingMode(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called (PC: 0x%08X).\r\n",
		                         functionId, context.m_State.nPC);
		break;
	}
}

bool CLoadcore::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x00:
		return LoadModule(args, argsSize, ret, retSize);
		break;
	case 0x01:
		LoadExecutable(args, argsSize, ret, retSize);
		break;
	case 0x06:
		LoadModuleFromMemory(args, argsSize, ret, retSize);
		return false; //Block EE till module is loaded
		break;
	case 0x07:
		return StopModule(args, argsSize, ret, retSize);
		break;
	case 0x08:
		UnloadModule(args, argsSize, ret, retSize);
		break;
	case 0x09:
		SearchModuleByName(args, argsSize, ret, retSize);
		break;
	case 0xFF:
		//This is sometimes called after binding this server with a client
		Initialize(args, argsSize, ret, retSize);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Invoking unknown function %d.\r\n", method);
		break;
	}
	return true;
}

void CLoadcore::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_VERSION_XML));
	m_moduleVersion = registerFile.GetRegister32(STATE_VERSION_MODULEVERSION);
}

void CLoadcore::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = new CRegisterStateFile(STATE_VERSION_XML);
	registerFile->SetRegister32(STATE_VERSION_MODULEVERSION, m_moduleVersion);
	archive.InsertFile(registerFile);
}

void CLoadcore::SetLoadExecutableHandler(const LoadExecutableHandler& loadExecutableHandler)
{
	m_loadExecutableHandler = loadExecutableHandler;
}

uint32 CLoadcore::GetLibraryEntryTable()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETLIBRARYENTRYTABLE "();\r\n");
	CLog::GetInstance().Warn(LOG_NAME, FUNCTION_GETLIBRARYENTRYTABLE " is not implemented.\r\n");
	return 0;
}

uint32 CLoadcore::RegisterLibraryEntries(uint32 exportTablePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERLIBRARYENTRIES "(exportTable = 0x%08X);\r\n", exportTablePtr);
	uint32* exportTable = reinterpret_cast<uint32*>(&m_ram[exportTablePtr]);
	auto module = std::make_shared<CDynamic>(exportTable);
	bool registered = m_bios.RegisterModule(module);
	if(!registered)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Failed to register library '%s'.\r\n", module->GetId().c_str());
	}
	return 0;
}

int32 CLoadcore::ReleaseLibraryEntries(uint32 exportTablePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_RELEASELIBRARYENTRIES "(exportTable = 0x%08X);\r\n", exportTablePtr);
	uint32* exportTable = reinterpret_cast<uint32*>(&m_ram[exportTablePtr]);
	auto moduleName = CDynamic::GetDynamicModuleName(exportTable);
	bool released = m_bios.ReleaseModule(moduleName);
	if(!released)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Failed to release library '%s'.\r\n", moduleName.c_str());
	}
	return 0;
}

uint32 CLoadcore::QueryBootMode(uint32 param)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_QUERYBOOTMODE "(param = %d);\r\n", param);
	return 0;
}

uint32 CLoadcore::SetRebootTimeLibraryHandlingMode(uint32 libAddr, uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETREBOOTTIMELIBHANDLINGMODE "(libAddr = 0x%08X, mode = 0x%08X);\r\n",
	                          libAddr, mode);
	return 0;
}

bool CLoadcore::LoadModule(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	char moduleName[PATH_MAX_SIZE];
	char moduleArgs[ARGS_MAX_SIZE];

	assert(argsSize == 512);

	//Sometimes called with 4, sometimes 8
	assert(retSize >= 4);

	uint32 moduleArgsSize = args[0];

	memcpy(moduleName, reinterpret_cast<const char*>(args) + 8, PATH_MAX_SIZE);
	memcpy(moduleArgs, reinterpret_cast<const char*>(args) + 8 + PATH_MAX_SIZE, ARGS_MAX_SIZE);

	//Load the module
	CLog::GetInstance().Print(LOG_NAME, "Request to load module '%s' received with %d bytes arguments payload.\r\n", moduleName, moduleArgsSize);

	auto moduleId = m_bios.LoadModule(moduleName);
	if(moduleId >= 0)
	{
		moduleId = m_bios.StartModule(moduleId, moduleName, moduleArgs, moduleArgsSize);
	}

	//This function returns something negative upon failure
	ret[0] = moduleId;

	if((moduleId >= 0) && !m_bios.IsModuleHle(moduleId))
	{
		//Block EE till the IOP has completed the operation and sends its reply to the EE
		return false;
	}
	else
	{
		//Loading module failed or is module is HLE, reply can be sent over immediately
		return true;
	}
}

void CLoadcore::LoadExecutable(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	char moduleName[PATH_MAX_SIZE];
	char sectionName[ARGS_MAX_SIZE];

	assert(argsSize == 512);
	assert(retSize >= 8);

	memcpy(moduleName, reinterpret_cast<const char*>(args) + 8, PATH_MAX_SIZE);
	memcpy(sectionName, reinterpret_cast<const char*>(args) + 8 + PATH_MAX_SIZE, ARGS_MAX_SIZE);

	CLog::GetInstance().Print(LOG_NAME, "Request to load section '%s' from executable '%s' received.\r\n", sectionName, moduleName);

	uint32 result = 0;

	//Load executable in EE memory
	if(m_loadExecutableHandler)
	{
		result = m_loadExecutableHandler(moduleName, sectionName);
	}

	//This function returns something negative upon failure
	ret[0] = result;     //epc or result (if negative)
	ret[1] = 0x00000000; //gp
}

void CLoadcore::LoadModuleFromMemory(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	const char* moduleArgs = reinterpret_cast<const char*>(args) + 8 + PATH_MAX_SIZE;
	uint32 moduleArgsSize = args[1];
	CLog::GetInstance().Print(LOG_NAME, "Request to load module at 0x%08X received with %d bytes arguments payload.\r\n", args[0], moduleArgsSize);
	auto moduleId = m_bios.LoadModule(args[0]);
	if(moduleId >= 0)
	{
		moduleId = m_bios.StartModule(moduleId, "", moduleArgs, moduleArgsSize);
	}
	ret[0] = moduleId;
	ret[1] = 0; //Result of module's start() function
}

bool CLoadcore::StopModule(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	char moduleArgs[ARGS_MAX_SIZE];

	assert(argsSize == 512);
	assert(retSize >= 4);

	uint32 moduleId = args[0];
	uint32 moduleArgsSize = args[1];

	memcpy(moduleArgs, reinterpret_cast<const char*>(args) + 8 + PATH_MAX_SIZE, ARGS_MAX_SIZE);

	CLog::GetInstance().Print(LOG_NAME, "StopModule(moduleId = %d, args, argsSize = 0x%08X);\r\n",
	                          moduleId, moduleArgsSize);

	if(!m_bios.CanStopModule(moduleId))
	{
		//Module is not stoppable (could be HLE module)
		ret[0] = 0;
		return true;
	}

	auto result = m_bios.StopModule(moduleId);
	ret[0] = result;

	if(result >= 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CLoadcore::UnloadModule(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	assert(argsSize == 4);
	assert(retSize >= 4);

	uint32 moduleId = args[0];

	CLog::GetInstance().Print(LOG_NAME, "UnloadModule(moduleId = %d);\r\n", moduleId);

	auto result = m_bios.UnloadModule(moduleId);
	ret[0] = result;
}

void CLoadcore::SearchModuleByName(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	assert(argsSize >= 0x200);
	assert(retSize >= 4);

	const char* moduleName = reinterpret_cast<const char*>(args) + 8;
	CLog::GetInstance().Print(LOG_NAME, "SearchModuleByName('%s');\r\n", moduleName);
	auto moduleId = m_bios.SearchModuleByName(moduleName);
	ret[0] = moduleId;
}

void CLoadcore::Initialize(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	assert(argsSize == 0);
	assert(retSize == 4);

	if(m_moduleVersion == 2020)
	{
		//Return '2020'
		//This is needed by Super Bust-A-Move
		ret[0] = 0x30323032;
	}
	else if(m_moduleVersion == 2000)
	{
		//Return '2000'
		//This is needed by 4x4 Evo
		ret[0] = 0x30303032;
	}
	else
	{
		//Return '....'
		ret[0] = 0x2E2E2E2E;
	}
}
