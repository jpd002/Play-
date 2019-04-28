#include "Iop_Thfpool.h"
#include "../Log.h"

#define LOG_NAME ("iop_thfpool")

using namespace Iop;

#define FUNCTION_CREATEFPL "CreateFpl"
#define FUNCTION_ALLOCATEFPL "AllocateFpl"
#define FUNCTION_PALLOCATEFPL "pAllocateFpl"
#define FUNCTION_FREEFPL "FreeFpl"

CThfpool::CThfpool(CIopBios& bios)
    : m_bios(bios)
{
}

std::string CThfpool::GetId() const
{
	return "thfpool";
}

std::string CThfpool::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATEFPL;
		break;
	case 6:
		return FUNCTION_ALLOCATEFPL;
		break;
	case 7:
		return FUNCTION_PALLOCATEFPL;
		break;
	case 9:
		return FUNCTION_FREEFPL;
		break;
	default:
		return "unknown";
		break;
	}
}

void CThfpool::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateFpl(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(AllocateFpl(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(pAllocateFpl(
			context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 9:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(FreeFpl(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called at (%08X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThfpool::CreateFpl(uint32 paramPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CREATEFPL "(paramPtr = 0x%08X);\r\n",
	                          paramPtr);
	return m_bios.CreateFpl(paramPtr);
}

uint32 CThfpool::AllocateFpl(uint32 fplId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCATEFPL "(fplId = %d);\r\n",
	                          fplId);
	return m_bios.AllocateFpl(fplId);
}

uint32 CThfpool::pAllocateFpl(uint32 fplId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_PALLOCATEFPL "(fplId = %d);\r\n",
	                          fplId);
	return m_bios.pAllocateFpl(fplId);
}

uint32 CThfpool::FreeFpl(uint32 fplId, uint32 blockPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_FREEFPL "(fplId = %d, blockPtr = 0x%08X);\r\n",
	                          fplId, blockPtr);
	return m_bios.FreeFpl(fplId, blockPtr);
}
