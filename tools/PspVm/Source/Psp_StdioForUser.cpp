#include "Psp_StdioForUser.h"
#include "Psp_IoFileMgrForUser.h"
#include "Log.h"

#define LOGNAME ("Psp_StdioForUser")

using namespace Psp;

CStdioForUser::CStdioForUser()
{

}

CStdioForUser::~CStdioForUser()
{

}

std::string CStdioForUser::GetName() const
{
	return "StdioForUser";
}

void CStdioForUser::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0xA6BAB2E9:
		//Stdout
		context.m_State.nGPR[CMIPS::V0].nV0 = CIoFileMgrForUser::FD_STDOUT;
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X.\r\n", methodId);
		break;
	}
}
