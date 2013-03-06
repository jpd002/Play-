#include <assert.h>
#include "Iop_Dmac.h"
#include "Iop_Intc.h"
#include "../Log.h"

#define LOG_NAME ("iop_dmac")

using namespace Iop;
using namespace Iop::Dmac;

CDmac::CDmac(uint8* ram, CIntc& intc) 
: m_ram(ram)
, m_intc(intc)
, m_channelSpu0(CH4_BASE, 4, *this)
, m_channelSpu1(CH8_BASE, 8, *this)
{
	memset(m_channel, 0, sizeof(m_channel));
	m_channel[4] = &m_channelSpu0;
	m_channel[8] = &m_channelSpu1;
	Reset();
}

CDmac::~CDmac()
{

}

void CDmac::Reset()
{
	m_DPCR = 0;
	m_DICR = 0;

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CChannel* channel(m_channel[i]);
		if(!channel) continue;
		channel->Reset();
	}
}

void CDmac::SetReceiveFunction(unsigned int channelId, const CChannel::ReceiveFunctionType& handler)
{
	assert(channelId < MAX_CHANNEL);
	if(channelId >= MAX_CHANNEL) return;
	CChannel* channel(m_channel[channelId]);
	if(channel)
	{
		channel->SetReceiveFunction(handler);
	}
}

unsigned int CDmac::GetChannelIdFromAddress(uint32 address)
{
	unsigned int channelId = -1;
	if(address >= DMAC_ZONE2_START)
	{
		channelId = (address - DMAC_ZONE2_START) / 0x10;
		channelId += 8;
	}
	else if(address >= CH0_BASE && address < DPCR)
	{
		channelId = (address - DMAC_ZONE1_START) / 0x10;
	}
	return channelId;
}

CChannel* CDmac::GetChannelFromAddress(uint32 address)
{
	unsigned int channelId = GetChannelIdFromAddress(address);
	assert(channelId < MAX_CHANNEL);
	if(channelId >= MAX_CHANNEL) return NULL;
	return m_channel[channelId];
}

void CDmac::ResumeDma(unsigned int channelIdx)
{
	auto channel = m_channel[channelIdx];
	assert(channel != nullptr);
	if(channel == nullptr) return;
	channel->ResumeDma();
}

void CDmac::AssertLine(unsigned int line)
{
	if(line < 7)
	{
		m_DICR |= 1 << (line + 24);
	}
	m_intc.AssertLine(CIntc::LINE_DMAC);
	m_intc.AssertLine(CIntc::LINE_DMA_BASE + line);
}

uint8* CDmac::GetRam()
{
	return m_ram;
}

uint32 CDmac::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	LogRead(address);
#endif
	switch(address)
	{
	case DPCR:
		return m_DPCR;
		break;
	case DICR:
		return m_DICR;
		break;
	default:
		{
			CChannel* channel(GetChannelFromAddress(address));
			if(channel)
			{
				return channel->ReadRegister(address);
			}
		}
		break;
	}
	return 0;
}

uint32 CDmac::WriteRegister(uint32 address, uint32 value)
{
#ifdef _DEBUG
	LogWrite(address, value);
#endif
	switch(address)
	{
	case DPCR:
		m_DPCR = value;
		break;
	case DICR:
		m_DICR &= 0xFF000000;
		m_DICR |= value;
		m_DICR &= ~(value & 0xFF000000);
		break;
	default:
		{
			CChannel* channel(GetChannelFromAddress(address));
			if(channel)
			{
				channel->WriteRegister(address, value);
			}
		}
		break;
	}
	return 0;
}

void CDmac::LogRead(uint32 address)
{
	switch(address)
	{
	case DPCR:
		CLog::GetInstance().Print(LOG_NAME, "= DPCR.\r\n");
		break;
	case DICR:
		CLog::GetInstance().Print(LOG_NAME, "= DICR.\r\n");
		break;
	default:
		{
			unsigned int channelId = GetChannelIdFromAddress(address);
			unsigned int registerId = address & 0xF;
			switch(registerId)
			{
			case CChannel::REG_MADR:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: = MADR.\r\n", channelId);
				break;
			case CChannel::REG_CHCR:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: = CHCR.\r\n", channelId);
				break;
			default:
				CLog::GetInstance().Print(LOG_NAME, "Read an unknown register 0x%0.8X.\r\n",
					address);
				break;
			}
		}
		break;
	}
}

void CDmac::LogWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	case DPCR:
		CLog::GetInstance().Print(LOG_NAME, "DPCR = 0x%0.8X.\r\n", value);
		break;
	case DICR:
		CLog::GetInstance().Print(LOG_NAME, "DICR = 0x%0.8X.\r\n", value);
		break;
	default:
		{
			unsigned int channelId = GetChannelIdFromAddress(address);
			unsigned int registerId = address & 0xF;
			switch(registerId)
			{
			case CChannel::REG_MADR:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: MADR = 0x%0.8X.\r\n", channelId, value);
				break;
			case CChannel::REG_BCR:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: BCR = 0x%0.8X.\r\n", channelId, value);
				break;
			case CChannel::REG_BCR + 2:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: BCR.ba = 0x%0.8X.\r\n", channelId, value);
				break;
			case CChannel::REG_CHCR:
				CLog::GetInstance().Print(LOG_NAME, "ch%0.2d: CHCR = 0x%0.8X.\r\n", channelId, value);
				break;
			default:
				CLog::GetInstance().Print(LOG_NAME, "Wrote 0x%0.8X to unknown register 0x%0.8X.\r\n",
					value, address);
				break;
			}
		}
		break;
	}
}
