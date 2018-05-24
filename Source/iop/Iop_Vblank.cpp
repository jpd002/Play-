#include "Iop_Vblank.h"
#include "IopBios.h"
#include "Iop_Intc.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME "iop_vblank"

#define FUNCTION_WAITVBLANKSTART "WaitVblankStart"
#define FUNCTION_WAITVBLANKEND "WaitVblankEnd"
#define FUNCTION_WAITVBLANK "WaitVblank"
#define FUNCTION_REGISTERVBLANKHANDLER "RegisterVblankHandler"

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
	case 8:
		return FUNCTION_REGISTERVBLANKHANDLER;
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
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = RegisterVblankHandler(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
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
	//TODO: Skip waiting if we're already in Vblank
	m_bios.SleepThreadTillVBlankStart();
	return 0;
}

int32 CVblank::RegisterVblankHandler(CMIPS& context, uint32 startEnd, uint32 priority, uint32 handlerPtr, uint32 handlerParam)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERVBLANKHANDLER "(startEnd = %d, priority = %d, handler = 0x%08X, arg = 0x%08X).\r\n",
	                          startEnd, priority, handlerPtr, handlerParam);
#endif
	uint32 intrLine = startEnd ? CIntc::LINE_EVBLANK : CIntc::LINE_VBLANK;

	m_bios.RegisterIntrHandler(intrLine, 0, handlerPtr, handlerParam);

	uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
	mask |= (1 << intrLine);
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);

	return 0;
}
