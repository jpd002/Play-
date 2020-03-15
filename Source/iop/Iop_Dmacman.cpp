#include "Iop_Dmacman.h"
#include "Iop_Dmac.h"
#include "Iop_DmacChannel.h"
#include "../Log.h"

#define LOGNAME "iop_dmacman"

using namespace Iop;

#define FUNCTIONID_DMACREQUEST 28
#define FUNCTIONID_DMACTRANSFER 32

#define FUNCTION_DMACREQUEST "DmacRequest"
#define FUNCTION_DMACTRANSFER "DmacTransfer"

#define INVALID_CHANNEL_BASE (-1)

static uint32 GetChannelBase(uint32 channel)
{
	uint32 channelBase = 0;
	switch(channel)
	{
	case CDmac::CHANNEL_SIO2in:
		return CDmac::CH11_BASE;
	case CDmac::CHANNEL_SIO2out:
		return CDmac::CH12_BASE;
	default:
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
	case FUNCTIONID_DMACREQUEST:
		return FUNCTION_DMACREQUEST;
	case FUNCTIONID_DMACTRANSFER:
		return FUNCTION_DMACTRANSFER;
	default:
		return "unknown";
		break;
	}
}

void CDmacman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
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
	default:
		CLog::GetInstance().Warn(LOGNAME, "%08X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
		break;
	}
}

uint32 CDmacman::DmacRequest(CMIPS& context, uint32 channel, uint32 addr, uint32 size, uint32 count, uint32 dir)
{
	CLog::GetInstance().Print(LOGNAME, FUNCTION_DMACREQUEST "(channel = %d, address = 0x%08X, size = 0x%08X, count = 0x%08X, dir = %d);\r\n",
		channel, addr, size, count, dir);
	assert(size < 0x10000);
	assert(count < 0x10000);
	
	uint32 channelBase = GetChannelBase(channel);
	if(channelBase == INVALID_CHANNEL_BASE) return 0;

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
	if(channelBase == INVALID_CHANNEL_BASE) return;

	context.m_pMemoryMap->SetWord(channelBase + Dmac::CChannel::REG_CHCR, chcr);
}
