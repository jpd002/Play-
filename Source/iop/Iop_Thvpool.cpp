#include "Iop_Thvpool.h"
#include "../Log.h"

#define LOG_NAME ("iop_thvpool")

using namespace Iop;

#define FUNCTION_CREATEVPL "CreateVpl"
#define FUNCTION_DELETEVPL "DeleteVpl"
#define FUNCTION_PALLOCATEVPL "pAllocateVpl"
#define FUNCTION_FREEVPL "FreeVpl"
#define FUNCTION_REFERVPLSTATUS "ReferVplStatus"

CThvpool::CThvpool(CIopBios& bios)
    : m_bios(bios)
{
}

std::string CThvpool::GetId() const
{
	return "thvpool";
}

std::string CThvpool::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATEVPL;
		break;
	case 5:
		return FUNCTION_DELETEVPL;
		break;
	case 7:
		return FUNCTION_PALLOCATEVPL;
		break;
	case 9:
		return FUNCTION_FREEVPL;
		break;
	case 11:
		return FUNCTION_REFERVPLSTATUS;
		break;
	default:
		return "unknown";
		break;
	}
}

void CThvpool::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateVpl(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DeleteVpl(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(pAllocateVpl(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	case 9:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(FreeVpl(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	case 11:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ReferVplStatus(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called at (%08X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThvpool::CreateVpl(uint32 paramPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CREATEVPL "(paramPtr = 0x%08X);\r\n",
	                          paramPtr);
	return m_bios.CreateVpl(paramPtr);
}

uint32 CThvpool::DeleteVpl(uint32 vplId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_DELETEVPL "(vplId = %d);\r\n",
	                          vplId);
	return m_bios.DeleteVpl(vplId);
}

uint32 CThvpool::pAllocateVpl(uint32 vplId, uint32 size)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_PALLOCATEVPL "(vplId = %d, size = 0x%08X);\r\n",
	                          vplId, size);
	return m_bios.pAllocateVpl(vplId, size);
}

uint32 CThvpool::FreeVpl(uint32 vplId, uint32 ptr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_FREEVPL "(vplId = %d, ptr = 0x%08X);\r\n",
	                          vplId, ptr);
	return m_bios.FreeVpl(vplId, ptr);
}

uint32 CThvpool::ReferVplStatus(uint32 vplId, uint32 statPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REFERVPLSTATUS "(vplId = %d, statPtr = 0x%08X);\r\n",
	                          vplId, statPtr);
	return m_bios.ReferVplStatus(vplId, statPtr);
}
