#include "Psp_ThreadManForUser.h"
#include "Log.h"

#define LOGNAME ("Psp_ThreadManForUser")

using namespace Psp;

CThreadManForUser::CThreadManForUser(CBios& bios, uint8* ram)
: m_bios(bios)
, m_ram(ram)
{

}

CThreadManForUser::~CThreadManForUser()
{

}

std::string CThreadManForUser::GetName() const
{
	return "ThreadManForUser";
}

uint32 CThreadManForUser::CreateThread(uint32 nameAddr, uint32 threadProcAddr, uint32 initPriority, uint32 stackSize, uint32 creationFlags, uint32 optParam)
{
	assert(optParam == 0);
	assert(stackSize >= 512);
	uint32 result = -1;
	const char* threadName = reinterpret_cast<const char*>(m_ram + nameAddr);
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "CreateThread(name = '%s', threadProcAddr = 0x%0.8X, initPriority = %d, stackSize = 0x%0.8X, creationFlags = 0x%0.8X, optParam = 0x%0.8X);\r\n",
		threadName, threadProcAddr, initPriority, stackSize, creationFlags, optParam);
#endif
	result = m_bios.CreateThread(threadName, threadProcAddr, initPriority, stackSize, creationFlags, optParam);
	return result;
}

uint32 CThreadManForUser::StartThread(uint32 threadId, uint32 argsSize, uint32 argsPtr)
{
	uint32 result = 0;
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "StartThread(threadId = %d, argsSize = 0x%0.8X, argsPtr = 0x%0.8X);\r\n",
		threadId, argsSize, argsPtr);
#endif
	m_bios.StartThread(threadId, argsSize, (argsPtr == 0) ? NULL : (m_ram + argsPtr));
	return result;
}

void CThreadManForUser::Invoke(uint32 methodId, CMIPS& ctx)
{
	switch(methodId)
	{
	case 0x446D8DE6:
		ctx.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateThread(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0,
			ctx.m_State.nGPR[CMIPS::A3].nV0,
			ctx.m_State.nGPR[CMIPS::T0].nV0,
			ctx.m_State.nGPR[CMIPS::T1].nV0));
		break;
	case 0xF475845D:
		ctx.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(StartThread(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0));
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X\r\n", methodId);
		break;
	}
}
