#include "Iop_Thevent.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME ("iop_thevent")

#define FUNCTION_CREATEEVENTFLAG			"CreateEventFlag"
#define FUNCTION_SETEVENTFLAG				"SetEventFlag"
#define FUNCTION_ISETEVENTFLAG				"iSetEventFlag"
#define FUNCTION_CLEAREVENTFLAG				"ClearEventFlag"
#define FUNCTION_WAITEVENTFLAG				"WaitEventFlag"

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
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATEEVENTFLAG;
		break;
	case 6:
		return FUNCTION_SETEVENTFLAG;
		break;
	case 7:
		return FUNCTION_ISETEVENTFLAG;
		break;
	case 8:
		return FUNCTION_CLEAREVENTFLAG;
		break;
	case 10:
		return FUNCTION_WAITEVENTFLAG;
		break;
	default:
		return "unknown";
		break;
	}
}

void CThevent::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CreateEventFlag(
			reinterpret_cast<EVENT*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
			));
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SetEventFlag(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(iSetEventFlag(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ClearEventFlag(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	case 10:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(WaitEventFlag(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0
			));
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called (%0.8X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThevent::CreateEventFlag(EVENT* eventPtr)
{
	return m_bios.CreateEventFlag(eventPtr->attributes, eventPtr->options, eventPtr->initValue);
}

uint32 CThevent::SetEventFlag(uint32 eventId, uint32 bits)
{
	return m_bios.SetEventFlag(eventId, bits, false);
}

uint32 CThevent::iSetEventFlag(uint32 eventId, uint32 bits)
{
	return m_bios.SetEventFlag(eventId, bits, true);
}

uint32 CThevent::ClearEventFlag(uint32 eventId, uint32 bits)
{
	return m_bios.ClearEventFlag(eventId, bits);
}

uint32 CThevent::WaitEventFlag(uint32 eventId, uint32 bits, uint32 mode, uint32 resultPtr)
{
	return m_bios.WaitEventFlag(eventId, bits, mode, resultPtr);
}
