#include "Iop_Thevent.h"
#include "../Log.h"

#define LOG_NAME ("iop_thevent")

using namespace Iop;

CThevent::CThevent(CIopBios& bios, uint8* ram) 
: m_bios(bios)
, m_ram(ram)
{

}

CThevent::~CThevent()
{

}

std::string CThevent::GetId() const
{
	return "thevent";
}

std::string CThevent::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CThevent::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(
			CreateEventFlag(reinterpret_cast<EVENT*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0]))
			);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called (%0.8X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThevent::CreateEventFlag(EVENT* eventPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "CreateEventFlag(eventPtr);.\r\n");
#endif
	return 3;
}
