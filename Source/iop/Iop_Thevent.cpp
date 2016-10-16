#include "Iop_Thevent.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME ("iop_thevent")

#define FUNCTION_CREATEEVENTFLAG			"CreateEventFlag"
#define FUNCTION_DELETEEVENTFLAG			"DeleteEventFlag"
#define FUNCTION_SETEVENTFLAG				"SetEventFlag"
#define FUNCTION_ISETEVENTFLAG				"iSetEventFlag"
#define FUNCTION_CLEAREVENTFLAG				"ClearEventFlag"
#define FUNCTION_ICLEAREVENTFLAG			"iClearEventFlag"
#define FUNCTION_WAITEVENTFLAG				"WaitEventFlag"
#define FUNCTION_REFEREVENTFLAGSTATUS		"ReferEventFlagStatus"
#define FUNCTION_IREFEREVENTFLAGSTATUS		"iReferEventFlagStatus"

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
	case 5:
		return FUNCTION_DELETEEVENTFLAG;
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
	case 9:
		return FUNCTION_ICLEAREVENTFLAG;
		break;
	case 10:
		return FUNCTION_WAITEVENTFLAG;
		break;
	case 13:
		return FUNCTION_REFEREVENTFLAGSTATUS;
		break;
	case 14:
		return FUNCTION_IREFEREVENTFLAGSTATUS;
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
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DeleteEventFlag(
			context.m_State.nGPR[CMIPS::A0].nV0
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
	case 9:
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
	case 13:
	case 14:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ReferEventFlagStatus(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
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

uint32 CThevent::DeleteEventFlag(uint32 eventId)
{
	return m_bios.DeleteEventFlag(eventId);
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

uint32 CThevent::ReferEventFlagStatus(uint32 eventId, uint32 infoPtr)
{
	return m_bios.ReferEventFlagStatus(eventId, infoPtr);
}
