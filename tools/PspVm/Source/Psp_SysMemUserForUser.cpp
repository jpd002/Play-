#include "Psp_SysMemUserForUser.h"
#include "Log.h"

#define LOGNAME ("Psp_SysMemUserForUser")

using namespace Psp;

CSysMemUserForUser::CSysMemUserForUser()
{

}

CSysMemUserForUser::~CSysMemUserForUser()
{

}

std::string CSysMemUserForUser::GetName() const
{
	return "SysMemUserForUser";
}

void CSysMemUserForUser::SetCompilerVersion(uint32 version)
{
	printf("Compiled using: gcc v%d.%d.%d\r\n", (version >> 16) & 0xFF, (version >> 8) & 0xFF, (version & 0xFF));
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetCompilerVersion(version = 0x%0.8X);\r\n", version);
#endif
}

void CSysMemUserForUser::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0xF77D77CB:
		SetCompilerVersion(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X.\r\n", methodId);
		break;
	}
}
