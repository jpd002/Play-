#include "Iop_NamcoAcAta.h"
#include "Log.h"
#include "../IopBios.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_acata")

#define FUNCTION_ATASETUP "AtaSetup"
#define FUNCTION_ATAREQUEST "AtaRequest"

struct ATACONTEXT
{
	uint32 doneFctPtr;
	uint32 param;
};

CAcAta::CAcAta(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
}

std::string CAcAta::GetId() const
{
	return "acata";
}

std::string CAcAta::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 8:
		return FUNCTION_ATASETUP;
	case 9:
		return FUNCTION_ATAREQUEST;
	default:
		return "unknown";
	}
}

void CAcAta::Invoke(CMIPS& ctx, unsigned int functionId)
{
	switch(functionId)
	{
	case 8:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = AtaSetup(
		    ctx.m_State.nGPR[CMIPS::A0].nV0,
		    ctx.m_State.nGPR[CMIPS::A1].nV0,
		    ctx.m_State.nGPR[CMIPS::A2].nV0,
		    ctx.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 9:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = AtaRequest(
		    ctx.m_State.nGPR[CMIPS::A0].nV0,
		    ctx.m_State.nGPR[CMIPS::A1].nV0,
		    ctx.m_State.nGPR[CMIPS::A2].nV0,
		    ctx.m_State.nGPR[CMIPS::A3].nV0,
		    ctx.m_pMemoryMap->GetWord(ctx.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
		    ctx.m_pMemoryMap->GetWord(ctx.m_State.nGPR[CMIPS::SP].nV0 + 0x14));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown IOP method invoked (0x%08X).\r\n", functionId);
		break;
	}
}

int32 CAcAta::AtaSetup(uint32 ataCtxPtr, uint32 doneFctPtr, uint32 param, uint32 timeout)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ATASETUP "(ataCtxPtr = 0x%08X, doneFctPtr = 0x%08X, param = 0x%08X, timeout = %d);",
	                          ataCtxPtr, doneFctPtr, param, timeout);

	assert(ataCtxPtr != 0);
	auto ataCtx = reinterpret_cast<ATACONTEXT*>(m_ram + ataCtxPtr);
	ataCtx->doneFctPtr = doneFctPtr;
	ataCtx->param = param;

	return ataCtxPtr;
}

int32 CAcAta::AtaRequest(uint32 ataCtxPtr, uint32 flag, uint32 commandPtr, uint32 item, uint32 buffer, uint32 size)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ATAREQUEST "(ataCtxPtr = 0x%08X, flag = 0x%08X, commandPtr = 0x%08X, item = %d, buffer = 0x%08X, size = %d);",
	                          ataCtxPtr, flag, commandPtr, item, buffer, size);

	auto ataCtx = reinterpret_cast<ATACONTEXT*>(m_ram + ataCtxPtr);
	auto command = reinterpret_cast<uint16*>(m_ram + commandPtr);

	uint32 result = 0;
	m_bios.TriggerCallback(ataCtx->doneFctPtr, ataCtxPtr, ataCtx->param, result);

	return 0;
}
