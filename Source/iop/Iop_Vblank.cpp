#include "Iop_Vblank.h"
#include "IopBios.h"
#include "Log.h"

using namespace Iop;

#define LOG_NAME "iop_vblank"

#define FUNCTION_WAITVBLANKSTART "WaitVblankStart"
#define FUNCTION_WAITVBLANKEND "WaitVblankEnd"
#define FUNCTION_WAITVBLANK "WaitVblank"
#define FUNCTION_WAITNONVBLANK "WaitNonVblank"
#define FUNCTION_REGISTERVBLANKHANDLER "RegisterVblankHandler"
#define FUNCTION_RELEASEVBLANKHANDLER "ReleaseVblankHandler"

CVblank::CVblank(CIopBios& bios)
    : m_bios(bios)
{
}

std::string CVblank::GetId() const
{
	return "vblank";
}

std::string CVblank::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_WAITVBLANKSTART;
		break;
	case 5:
		return FUNCTION_WAITVBLANKEND;
		break;
	case 6:
		return FUNCTION_WAITVBLANK;
		break;
	case 7:
		return FUNCTION_WAITNONVBLANK;
		break;
	case 8:
		return FUNCTION_REGISTERVBLANKHANDLER;
		break;
	case 9:
		return FUNCTION_RELEASEVBLANKHANDLER;
		break;
	default:
		return "unknown";
		break;
	}
}

void CVblank::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = WaitVblankStart();
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = WaitVblankEnd();
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = WaitVblank();
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = WaitNonVblank();
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = RegisterVblankHandler(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 9:
		context.m_State.nGPR[CMIPS::V0].nD0 = ReleaseVblankHandler(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function called (%d).\r\n", functionId);
		break;
	}
}

int32 CVblank::WaitVblankStart()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANKSTART "();\r\n");
#endif
	m_bios.SleepThreadTillVBlankStart();
	return 0;
}

int32 CVblank::WaitVblankEnd()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANKEND "();\r\n");
#endif
	m_bios.SleepThreadTillVBlankEnd();
	return 0;
}

int32 CVblank::WaitVblank()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANK "();\r\n");
#endif
	m_bios.SleepThreadTillVBlank();
	return 0;
}

int32 CVblank::WaitNonVblank()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITNONVBLANK "();\r\n");
#endif
	m_bios.SleepThreadTillNonVBlank();
	return 0;
}

int32 CVblank::RegisterVblankHandler(uint32 startEnd, uint32 priority, uint32 handlerPtr, uint32 handlerParam)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERVBLANKHANDLER "(startEnd = %d, priority = %d, handler = 0x%08X, arg = 0x%08X).\r\n",
	                          startEnd, priority, handlerPtr, handlerParam);
#endif
	return m_bios.RegisterVblankHandler(startEnd, priority, handlerPtr, handlerParam);
}

int32 CVblank::ReleaseVblankHandler(uint32 startEnd, uint32 handlerPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_RELEASEVBLANKHANDLER "(startEnd = %d, handler = 0x%08X).\r\n",
	                          startEnd, handlerPtr);
#endif
	return m_bios.ReleaseVblankHandler(startEnd, handlerPtr);
}
