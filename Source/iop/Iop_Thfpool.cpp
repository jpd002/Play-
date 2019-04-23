#include "Iop_Thfpool.h"
#include "../Log.h"

#define LOG_NAME ("iop_thfpool")

using namespace Iop;

#define FUNCTION_CREATEFPL "CreateFpl"
#define FUNCTION_ALLOCATEFPL "AllocateFpl"

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
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called at (%08X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThfpool::CreateFpl(uint32 paramPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CREATEFPL "(paramPtr = 0x%08X);\r\n",
	                          paramPtr);
	return 0x01234567;
	//return m_bios.CreateFpl(paramPtr);
}

uint32 CThfpool::AllocateFpl(uint32 fplId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCATEFPL "(fplId = %d);\r\n",
	                          fplId);
	//return m_bios.AllocateFpl(fplId);
	return 0;
}
