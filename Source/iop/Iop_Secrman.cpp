#include "Iop_Secrman.h"
#include "../Log.h"

#define LOGNAME "iop_secrman"

using namespace Iop;

#define FUNCTIONID_SETMCCOMMANDHANDLER 4
#define FUNCTIONID_SETMCDEVIDHANDLER 5
#define FUNCTIONID_AUTHCARD 6

#define FUNCTION_SETMCCOMMANDHANDLER "SetMcCommandHandler"
#define FUNCTION_SETMCDEVIDHANDLER "SetMcDevIdHandler"
#define FUNCTION_AUTHCARD "AuthCard"

std::string CSecrman::GetId() const
{
	return "secrman";
}

std::string CSecrman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case FUNCTIONID_SETMCCOMMANDHANDLER:
		return FUNCTION_SETMCCOMMANDHANDLER;
	case FUNCTIONID_SETMCDEVIDHANDLER:
		return FUNCTION_SETMCDEVIDHANDLER;
	case FUNCTIONID_AUTHCARD:
		return FUNCTION_AUTHCARD;
	default:
		return "unknown";
		break;
	}
}

void CSecrman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case FUNCTIONID_SETMCCOMMANDHANDLER:
		SetMcCommandHandler(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_SETMCDEVIDHANDLER:
		SetMcDevIdHandler(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_AUTHCARD:
		context.m_State.nGPR[CMIPS::V0].nV0 = AuthCard(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOGNAME, "%08X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
		break;
	}
}

void CSecrman::SetMcCommandHandler(uint32 handlerPtr)
{
	CLog::GetInstance().Print(LOGNAME, "SetMcCommandHandler(handlerPtr = 0x%08X);\r\n", handlerPtr);
}

void CSecrman::SetMcDevIdHandler(uint32 handlerPtr)
{
	CLog::GetInstance().Print(LOGNAME, "SetMcDevIdHandler(handlerPtr = 0x%08X);\r\n", handlerPtr);
}

uint32 CSecrman::AuthCard(uint32 port, uint32 slot, uint32 cnum)
{
	CLog::GetInstance().Print(LOGNAME, "AuthCard(port = %d, slot = %d, cnum = %d);\r\n",
	                          port, slot, cnum);
	//Returns non-zero on success
	return 1;
}
