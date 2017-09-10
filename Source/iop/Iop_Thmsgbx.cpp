#include "Iop_Thmsgbx.h"
#include "IopBios.h"
#include "../Log.h"

#define LOG_NAME ("iop_thmsgbx")

using namespace Iop;

#define FUNCTION_CREATEMBX			"CreateMbx"
#define FUNCTION_DELETEMBX			"DeleteMbx"
#define FUNCTION_SENDMBX			"SendMbx"
#define FUNCTION_ISENDMBX			"iSendMbx"
#define FUNCTION_RECEIVEMBX			"ReceiveMbx"
#define FUNCTION_POLLMBX			"PollMbx"
#define FUNCTION_REFERMBXSTATUS		"ReferMbxStatus"

CThmsgbx::CThmsgbx(CIopBios& bios, uint8* ram)
: m_bios(bios)
, m_ram(ram)
{

}

CThmsgbx::~CThmsgbx()
{

}

std::string CThmsgbx::GetId() const
{
	return "thmsgbx";
}

std::string CThmsgbx::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATEMBX;
		break;
	case 5:
		return FUNCTION_DELETEMBX;
		break;
	case 6:
		return FUNCTION_SENDMBX;
		break;
	case 7:
		return FUNCTION_ISENDMBX;
		break;
	case 8:
		return FUNCTION_RECEIVEMBX;
		break;
	case 9:
		return FUNCTION_POLLMBX;
		break;
	case 11:
		return FUNCTION_REFERMBXSTATUS;
		break;
	default:
		return "unknown";
		break;
	}
}

void CThmsgbx::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nV0 = CreateMbx(
			reinterpret_cast<MSGBX*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
			);
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nV0 = DeleteMbx(
			context.m_State.nGPR[CMIPS::A0].nV0
			);
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nV0 = SendMbx(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nV0 = iSendMbx(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
		);
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nV0 = ReceiveMbx(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	case 9:
		context.m_State.nGPR[CMIPS::V0].nV0 = PollMbx(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	case 11:
		context.m_State.nGPR[CMIPS::V0].nV0 = ReferMbxStatus(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called at (%08X).\r\n", functionId, context.m_State.nPC);
		break;
	}
}

uint32 CThmsgbx::CreateMbx(const MSGBX* msgBx)
{
	return m_bios.CreateMessageBox();
}

uint32 CThmsgbx::DeleteMbx(uint32 boxId)
{
	return m_bios.DeleteMessageBox(boxId);
}

uint32 CThmsgbx::SendMbx(uint32 boxId, uint32 messagePtr)
{
	return m_bios.SendMessageBox(boxId, messagePtr, false);
}

uint32 CThmsgbx::iSendMbx(uint32 boxId, uint32 messagePtr)
{
	return m_bios.SendMessageBox(boxId, messagePtr, true);
}

uint32 CThmsgbx::ReceiveMbx(uint32 messagePtr, uint32 boxId)
{
	return m_bios.ReceiveMessageBox(messagePtr, boxId);
}

uint32 CThmsgbx::PollMbx(uint32 messagePtr, uint32 boxId)
{
	return m_bios.PollMessageBox(messagePtr, boxId);
}

uint32 CThmsgbx::ReferMbxStatus(uint32 boxId, uint32 statusPtr)
{
	return m_bios.ReferMessageBoxStatus(boxId, statusPtr);
}
