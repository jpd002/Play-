#include "Psp_UmdUser.h"
#include "Log.h"

#define LOGNAME ("Psp_UmdUser")

using namespace Psp;

std::string CUmdUser::GetName() const
{
	return "sceUmdUser";
}

void CUmdUser::Invoke(uint32 methodId, CMIPS& ctx)
{
	switch(methodId)
	{
	case 0x4A9E5E29:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = UmdWaitDriveStateCB(
		    ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X.\r\n", methodId);
		break;
	}
}

int32 CUmdUser::UmdWaitDriveStateCB(uint32 state)
{
	CLog::GetInstance().Print(LOGNAME, "UmdWaitDriveStatCB(state = %d);\r\n", state);
	return 0;
}
