#include "Iop_Modload.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME		("iop_modload")

//TODO: Move this in CIopBios
#define KERNEL_RESULT_ERROR_UNKNOWN_MODULE -202

#define FUNCTION_LOADSTARTMODULE       "LoadStartModule"
#define FUNCTION_STARTMODULE           "StartModule"
#define FUNCTION_LOADMODULEBUFFER      "LoadModuleBuffer"
#define FUNCTION_GETMODULEIDLIST       "GetModuleIdList"
#define FUNCTION_REFERMODULESTATUS     "ReferModuleStatus"
#define FUNCTION_SEARCHMODULEBYNAME    "SearchModuleByName"

CModload::CModload(CIopBios& bios, uint8* ram)
: m_bios(bios)
, m_ram(ram)
{

}

std::string CModload::GetId() const
{
	return "modload";
}

std::string CModload::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 7:
		return FUNCTION_LOADSTARTMODULE;
		break;
	case 8:
		return FUNCTION_STARTMODULE;
		break;
	case 10:
		return FUNCTION_LOADMODULEBUFFER;
		break;
	case 16:
		return FUNCTION_GETMODULEIDLIST;
		break;
	case 17:
		return FUNCTION_REFERMODULESTATUS;
		break;
	case 22:
		return FUNCTION_SEARCHMODULEBYNAME;
		break;
	default:
		return "unknown";
		break;
	}
}

void CModload::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(LoadStartModule(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0
		));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(StartModule(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10)
		));
		break;
	case 10:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(LoadModuleBuffer(
			context.m_State.nGPR[CMIPS::A0].nV0
		));
		break;
	case 16:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetModuleIdList(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0
		));
		break;
	case 17:
		context.m_State.nGPR[CMIPS::V0].nD0 = ReferModuleStatus(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 22:
		context.m_State.nGPR[CMIPS::V0].nD0 = SearchModuleByName(
			context.m_State.nGPR[CMIPS::A0].nV0
		);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "(%0.8X): Unknown function (%d) called.\r\n", 
			context.m_State.nPC, functionId);
		break;
	}
}

uint32 CModload::LoadStartModule(uint32 pathPtr, uint32 argsLength, uint32 argsPtr, uint32 resultPtr)
{
	const char* path = reinterpret_cast<const char*>(m_ram + pathPtr);
	const char* args = reinterpret_cast<const char*>(m_ram + argsPtr);
	try
	{
		auto moduleId = m_bios.LoadModule(path);
		if(moduleId >= 0)
		{
			moduleId = m_bios.StartModule(moduleId, path, args, argsLength);
		}
		return moduleId;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error occured while trying to load module '%s' : %s\r\n", 
			path, except.what());
	}
	return -1;
}

uint32 CModload::StartModule(uint32 moduleId, uint32 pathPtr, uint32 argsLength, uint32 argsPtr, uint32 resultPtr)
{
	const char* path = reinterpret_cast<const char*>(m_ram + pathPtr);
	const char* args = reinterpret_cast<const char*>(m_ram + argsPtr);
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_STARTMODULE "(moduleId = %d, path = '%s', argsLength = %d, argsPtr = 0x%0.8X, resultPtr = 0x%0.8X);\r\n",
		moduleId, path, argsLength, argsPtr, resultPtr);
	auto result = m_bios.StartModule(moduleId, path, args, argsLength);
	return result;
}

uint32 CModload::LoadModuleBuffer(uint32 modBufPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_LOADMODULEBUFFER "(modBufPtr = 0x%0.8X);\r\n",
		modBufPtr);
	assert((modBufPtr & 0x03) == 0);
	auto result = m_bios.LoadModule(modBufPtr);
	return result;
}

uint32 CModload::GetModuleIdList(uint32 readBufPtr, uint32 readBufSize, uint32 moduleCountPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETMODULEIDLIST "(readBufPtr = 0x%0.8X, readBufSize = 0x%0.8X, moduleCountPtr = 0x%0.8X);\r\n",
		readBufPtr, readBufSize, moduleCountPtr);
	auto moduleCount = (moduleCountPtr != 0) ? reinterpret_cast<uint32*>(m_ram + moduleCountPtr) : nullptr;
	if(moduleCount)
	{
		(*moduleCount) = 0;
	}
	return 0;
}

int32 CModload::ReferModuleStatus(uint32 moduleId, uint32 moduleStatusPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REFERMODULESTATUS "(moduleId = %d, moduleStatusPtr = 0x%0.8X);\r\n",
		moduleId, moduleStatusPtr);
	return KERNEL_RESULT_ERROR_UNKNOWN_MODULE;
}

int32 CModload::SearchModuleByName(uint32 moduleNamePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SEARCHMODULEBYNAME "(moduleNamePtr = %s);\r\n",
		PrintStringParameter(m_ram, moduleNamePtr).c_str());
	return KERNEL_RESULT_ERROR_UNKNOWN_MODULE;
}
