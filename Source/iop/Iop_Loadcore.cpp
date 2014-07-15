#include "Iop_Loadcore.h"
#include "Iop_Dynamic.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME "iop_loadcore"

#define FUNCTION_FLUSHDCACHE			"FlushDcache"
#define FUNCTION_REGISTERLIBRARYENTRIES	"RegisterLibraryEntries"
#define FUNCTION_QUERYBOOTMODE			"QueryBootMode"

#define PATH_MAX_SIZE 252
#define ARGS_MAX_SIZE 252

CLoadcore::CLoadcore(CIopBios& bios, uint8* ram, CSifMan& sifMan)
: m_bios(bios)
, m_ram(ram)
{
	sifMan.RegisterModule(MODULE_ID, this);
}

CLoadcore::~CLoadcore()
{

}

std::string CLoadcore::GetId() const
{
	return "loadcore";
}

std::string CLoadcore::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 5:
		return FUNCTION_FLUSHDCACHE;
		break;
	case 6:
		return FUNCTION_REGISTERLIBRARYENTRIES;
		break;
	case 12:
		return FUNCTION_QUERYBOOTMODE;
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
	case 5:
		//FlushDCache
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(RegisterLibraryEntries(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	case 12:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(QueryBootMode(
			context.m_State.nGPR[CMIPS::A0].nV0
			));
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called (PC: 0x%0.8X).\r\n", 
			functionId, context.m_State.nPC);
		break;
	}
}

bool CLoadcore::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x00:
		LoadModule(args, argsSize, ret, retSize);
		return false;	//Block EE till module is loaded
		break;
	case 0x01:
		LoadExecutable(args, argsSize, ret, retSize);
		break;
	case 0x06:
		LoadModuleFromMemory(args, argsSize, ret, retSize);
		return false;	//Block EE till module is loaded
		break;
	case 0x09:
		SearchModuleByName(args, argsSize, ret, retSize);
		break;
	case 0xFF:
		//This is sometimes called after binding this server with a client
		Initialize(args, argsSize, ret, retSize);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Invoking unknown function %d.\r\n", method);
		break;
	}
	return true;
}

void CLoadcore::SetLoadExecutableHandler(const LoadExecutableHandler& loadExecutableHandler)
{
	m_loadExecutableHandler = loadExecutableHandler;
}

uint32 CLoadcore::RegisterLibraryEntries(uint32 exportTablePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERLIBRARYENTRIES "(exportTable = 0x%0.8X);\r\n", exportTablePtr);
	uint32* exportTable = reinterpret_cast<uint32*>(&m_ram[exportTablePtr]);
	m_bios.RegisterDynamicModule(new CDynamic(exportTable));
	return 0;
}

uint32 CLoadcore::QueryBootMode(uint32 param)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_QUERYBOOTMODE "(param = %d);\r\n", param);
	return 0;
}

void CLoadcore::LoadModule(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
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

	m_bios.LoadAndStartModule(moduleName, moduleArgs, moduleArgsSize);

	//This function returns something negative upon failure
	ret[0] = 0x00000000;
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
	ret[0] = result;		//epc or result (if negative)
	ret[1] = 0x00000000;	//gp
}

void CLoadcore::LoadModuleFromMemory(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	CLog::GetInstance().Print(LOG_NAME, "Request to load module at 0x%0.8X received with %d bytes arguments payload.\r\n", args[0], 0);
	m_bios.LoadAndStartModule(args[0], NULL, 0);
	ret[0] = 0x00000000;
}

void CLoadcore::SearchModuleByName(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	assert(argsSize >= 0x200);
	assert(retSize >= 4);

	const char* moduleName = reinterpret_cast<const char*>(args) + 8;
	CLog::GetInstance().Print(LOG_NAME, "SearchModuleByName('%s');\r\n", moduleName);

	if(m_bios.IsModuleLoaded(moduleName))
	{
		//Supposed to return some kind of id here...
		ret[0] = 0x01234567;
	}
	else
	{
		ret[0] = -1;
	}
}

void CLoadcore::Initialize(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize)
{
	assert(argsSize == 0);
	assert(retSize == 4);

	ret[0] = 0x2E2E2E2E;
}
