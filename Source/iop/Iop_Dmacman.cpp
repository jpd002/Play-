#include "Iop_Dmacman.h"
#include "Iop_Dmac.h"
#include "Iop_DmacChannel.h"
#include "../Log.h"

#define LOGNAME "iop_dmacman"

using namespace Iop;

#define FUNCTIONID_DMACSETDPCR 14
#define FUNCTIONID_DMACGETDPCR 15
#define FUNCTIONID_DMACSETDPCR2 16
#define FUNCTIONID_DMACGETDPCR2 17
#define FUNCTIONID_DMACSETDPCR3 18
#define FUNCTIONID_DMACGETDPCR3 19
#define FUNCTIONID_DMACREQUEST 28
#define FUNCTIONID_DMACTRANSFER 32
#define FUNCTIONID_DMACCHSETDPCR 33
#define FUNCTIONID_DMACENABLE 34
#define FUNCTIONID_DMACDISABLE 35

#define FUNCTION_DMACSETDPCR "DmacSetDpcr"
#define FUNCTION_DMACGETDPCR "DmacGetDpcr"
#define FUNCTION_DMACSETDPCR2 "DmacSetDpcr2"
#define FUNCTION_DMACGETDPCR2 "DmacGetDpcr2"
#define FUNCTION_DMACSETDPCR3 "DmacSetDpcr3"
#define FUNCTION_DMACGETDPCR3 "DmacGetDpcr3"
#define FUNCTION_DMACREQUEST "DmacRequest"
#define FUNCTION_DMACTRANSFER "DmacTransfer"
#define FUNCTION_DMACCHSETDPCR "DmacChSetDpcr"
#define FUNCTION_DMACENABLE "DmacEnable"
#define FUNCTION_DMACDISABLE "DmacDisable"

#define INVALID_CHANNEL_BASE (-1)

static uint32 GetChannelBase(uint32 channel)
{
	switch(channel)
	{
	case CDmac::CHANNEL_SIO2in:
		return CDmac::CH11_BASE;
	case CDmac::CHANNEL_SIO2out:
		return CDmac::CH12_BASE;
	default:
		CLog::GetInstance().Warn(LOGNAME, "Unknown channel (%d).\r\n", channel);
		assert(false);
		return INVALID_CHANNEL_BASE;
	}
}

std::string CDmacman::GetId() const
{
	return "dmacman";
}

std::string CDmacman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case FUNCTIONID_DMACSETDPCR:
		return FUNCTION_DMACSETDPCR;
	case FUNCTIONID_DMACGETDPCR:
		return FUNCTION_DMACGETDPCR;
	case FUNCTIONID_DMACSETDPCR2:
		return FUNCTION_DMACSETDPCR2;
	case FUNCTIONID_DMACGETDPCR2:
		return FUNCTION_DMACGETDPCR2;
	case FUNCTIONID_DMACSETDPCR3:
		return FUNCTION_DMACSETDPCR3;
	case FUNCTIONID_DMACGETDPCR3:
		return FUNCTION_DMACGETDPCR3;
	case FUNCTIONID_DMACREQUEST:
		return FUNCTION_DMACREQUEST;
	case FUNCTIONID_DMACTRANSFER:
		return FUNCTION_DMACTRANSFER;
	case FUNCTIONID_DMACCHSETDPCR:
		return FUNCTION_DMACCHSETDPCR;
	case FUNCTIONID_DMACENABLE:
		return FUNCTION_DMACENABLE;
	case FUNCTIONID_DMACDISABLE:
		return FUNCTION_DMACDISABLE;
	default:
		return "unknown";
		break;
	}
}

void CDmacman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case FUNCTIONID_DMACSETDPCR:
		DmacSetDpcr(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_DMACGETDPCR:
		context.m_State.nGPR[CMIPS::V0].nV0 = DmacGetDpcr(
		    context);
		break;
	case FUNCTIONID_DMACSETDPCR2:
		DmacSetDpcr2(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_DMACGETDPCR2:
		context.m_State.nGPR[CMIPS::V0].nV0 = DmacGetDpcr2(
		    context);
		break;
	case FUNCTIONID_DMACSETDPCR3:
		DmacSetDpcr3(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_DMACGETDPCR3:
		context.m_State.nGPR[CMIPS::V0].nV0 = DmacGetDpcr3(
		    context);
		break;
	case FUNCTIONID_DMACREQUEST:
		context.m_State.nGPR[CMIPS::V0].nV0 = DmacRequest(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0,
		    context.m_pMemoryMap->GetWord(context.m_State.nGPR[CMIPS::SP].nV0 + 0x10));
		break;
	case FUNCTIONID_DMACTRANSFER:
		DmacTransfer(context, context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_DMACCHSETDPCR:
		DmacChSetDpcr(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case FUNCTIONID_DMACENABLE:
		DmacEnable(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case FUNCTIONID_DMACDISABLE:
		DmacDisable(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOGNAME, "%08X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
		break;
	}
}

void CDmacman::DmacSetDpcr(CMIPS& context, uint32 value)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACSETDPCR "(value = 0x%08X);\r\n", value);
	context.m_pMemoryMap->SetWord(CDmac::DPCR, value);
}

uint32 CDmacman::DmacGetDpcr(CMIPS& context)
{
	return context.m_pMemoryMap->GetWord(CDmac::DPCR);
}

void CDmacman::DmacSetDpcr2(CMIPS& context, uint32 value)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACSETDPCR2 "(value = 0x%08X);\r\n", value);
	context.m_pMemoryMap->SetWord(CDmac::DPCR2, value);
}

uint32 CDmacman::DmacGetDpcr2(CMIPS& context)
{
	return context.m_pMemoryMap->GetWord(CDmac::DPCR2);
}

void CDmacman::DmacSetDpcr3(CMIPS& context, uint32 value)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACSETDPCR3 "(value = 0x%08X);\r\n", value);
	context.m_pMemoryMap->SetWord(CDmac::DPCR3, value);
}

uint32 CDmacman::DmacGetDpcr3(CMIPS& context)
{
	return context.m_pMemoryMap->GetWord(CDmac::DPCR3);
}

uint32 CDmacman::DmacRequest(CMIPS& context, uint32 channel, uint32 addr, uint32 size, uint32 count, uint32 dir)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACREQUEST "(channel = %d, address = 0x%08X, size = 0x%08X, count = 0x%08X, dir = %d);\r\n",
	                          channel, addr, size, count, dir);
	assert(size < 0x10000);
	assert(count < 0x10000);

	uint32 channelBase = GetChannelBase(channel);
	if(channelBase == INVALID_CHANNEL_BASE)
	{
		CLog::GetInstance().Warn(LOGNAME, "Received " FUNCTION_DMACREQUEST " for invalid channel %d\r\n", channel);
		return 0;
	}

	uint32 bcr = size | (count << 16);
	context.m_pMemoryMap->SetWord(channelBase + Dmac::CChannel::REG_MADR, addr);
	context.m_pMemoryMap->SetWord(channelBase + Dmac::CChannel::REG_BCR, bcr);
	return 1;
}

void CDmacman::DmacTransfer(CMIPS& context, uint32 channel)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACTRANSFER "(channel = %d);\r\n", channel);

	auto chcr = make_convertible<Dmac::CChannel::CHCR>(0);
	chcr.tr = 1;
	chcr.co = 1;
	chcr.dr = 1; //Direction?

	uint32 channelBase = GetChannelBase(channel);
	if(channelBase == INVALID_CHANNEL_BASE)
	{
		CLog::GetInstance().Warn(LOGNAME, "Received " FUNCTION_DMACTRANSFER " for invalid channel %d\r\n", channel);
		return;
	}

	context.m_pMemoryMap->SetWord(channelBase + Dmac::CChannel::REG_CHCR, chcr);
}

void CDmacman::DmacChSetDpcr(CMIPS& context, uint32 channel, uint32 value)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACCHSETDPCR "(channel = %d, value = 0x%08X);\r\n", channel, value);

	uint32 dpcrAddr = GetDPCRAddr(channel);

	uint32 dpcr = context.m_pMemoryMap->GetWord(dpcrAddr);
	uint32 mask = ~(0x7 << ((channel % 7) * 4));

	dpcr = (dpcr & mask) | ((value & 0x7) << ((channel % 7) * 4));

	context.m_pMemoryMap->SetWord(dpcrAddr, dpcr | mask);

	return;
}

void CDmacman::DmacEnable(CMIPS& context, uint32 channel)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACENABLE "(channel = %d);\r\n", channel);

	uint32 dpcrAddr = GetDPCRAddr(channel);

	uint32 dpcr = context.m_pMemoryMap->GetWord(dpcrAddr);

	uint32 mask = 0x8 << ((channel % 7) * 4);

	context.m_pMemoryMap->SetWord(dpcrAddr, dpcr | mask);

	return;
}

void CDmacman::DmacDisable(CMIPS& context, uint32 channel)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACDISABLE "(channel = %d);\r\n", channel);

	uint32 dpcrAddr = GetDPCRAddr(channel);

	uint32 dpcr = context.m_pMemoryMap->GetWord(dpcrAddr);

	uint32 mask = ~(0x8 << ((channel % 7) * 4));

	context.m_pMemoryMap->SetWord(dpcrAddr, dpcr & mask);

	return;
}

uint32 CDmacman::GetDPCRAddr(uint32 channel)
{
	// 0 - 6
	if(channel < 7)
	{
		return CDmac::DPCR;
	}
	// 7 - 12
	else if(channel < 13)
	{
		return CDmac::DPCR2;
	}
	// 13 - 15 (i.Link)
	else if(channel < 16)
	{
		return CDmac::DPCR3;
	}
	return 0;
}
