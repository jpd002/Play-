#include <assert.h>
#include <cstring>
#include "Iop_Dmac.h"
#include "Iop_Intc.h"
#include "../states/RegisterStateFile.h"
#include "../Log.h"

#define LOG_NAME ("iop_dmac")

#define STATE_REGS_XML ("iop_dmac/regs.xml")
#define STATE_REGS_DPCR ("DPCR")
#define STATE_REGS_DPCR2 ("DPCR2")
#define STATE_REGS_DICR ("DICR")

using namespace Iop;
using namespace Iop::Dmac;

CDmac::CDmac(uint8* ram, CIntc& intc)
    : m_ram(ram)
    , m_intc(intc)
    , m_channelSpu0(CH4_BASE, CHANNEL_SPU0, *this)
    , m_channelSpu1(CH8_BASE, CHANNEL_SPU1, *this)
    , m_channelDev9(CH9_BASE, CHANNEL_DEV9, *this)
    , m_channelSio2In(CH11_BASE, CHANNEL_SIO2in, *this)
    , m_channelSio2Out(CH12_BASE, CHANNEL_SIO2out, *this)
{
	memset(m_channel, 0, sizeof(m_channel));
	m_channel[CHANNEL_SPU0] = &m_channelSpu0;
	m_channel[CHANNEL_SPU1] = &m_channelSpu1;
	m_channel[CHANNEL_DEV9] = &m_channelDev9;
	//Not enabled, only used for testing purposes
	//m_channel[CHANNEL_SIO2in] = &m_channelSio2In;
	//m_channel[CHANNEL_SIO2out] = &m_channelSio2Out;
	Reset();
}

void CDmac::Reset()
{
	m_DPCR = 0;
	m_DPCR2 = 0;
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
	case DPCR2:
		return m_DPCR2;
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
	case DPCR2:
		m_DPCR2 = value;
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

void CDmac::LoadState(Framework::CZipArchiveReader& archive)
{
	{
		auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_REGS_XML));
		m_DPCR = registerFile.GetRegister32(STATE_REGS_DPCR);
		m_DPCR2 = registerFile.GetRegister32(STATE_REGS_DPCR2);
		m_DICR = registerFile.GetRegister32(STATE_REGS_DICR);
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto channel = m_channel[i];
		if(!channel) continue;
		channel->LoadState(archive);
	}
}

void CDmac::SaveState(Framework::CZipArchiveWriter& archive)
{
	{
		auto registerFile = new CRegisterStateFile(STATE_REGS_XML);
		registerFile->SetRegister32(STATE_REGS_DPCR, m_DPCR);
		registerFile->SetRegister32(STATE_REGS_DPCR2, m_DPCR2);
		registerFile->SetRegister32(STATE_REGS_DICR, m_DICR);
		archive.InsertFile(registerFile);
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto channel = m_channel[i];
		if(!channel) continue;
		channel->SaveState(archive);
	}
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
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: = MADR.\r\n", channelId);
			break;
		case CChannel::REG_CHCR:
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: = CHCR.\r\n", channelId);
			break;
		default:
			CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n",
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
		CLog::GetInstance().Print(LOG_NAME, "DPCR = 0x%08X.\r\n", value);
		break;
	case DICR:
		CLog::GetInstance().Print(LOG_NAME, "DICR = 0x%08X.\r\n", value);
		break;
	default:
	{
		unsigned int channelId = GetChannelIdFromAddress(address);
		unsigned int registerId = address & 0xF;
		switch(registerId)
		{
		case CChannel::REG_MADR:
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: MADR = 0x%08X.\r\n", channelId, value);
			break;
		case CChannel::REG_BCR:
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: BCR = 0x%08X.\r\n", channelId, value);
			break;
		case CChannel::REG_BCR + 2:
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: BCR.ba = 0x%08X.\r\n", channelId, value);
			break;
		case CChannel::REG_CHCR:
			CLog::GetInstance().Print(LOG_NAME, "ch%02d: CHCR = 0x%08X.\r\n", channelId, value);
			break;
		default:
			CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to unknown register 0x%08X.\r\n",
			                         value, address);
			break;
		}
	}
	break;
	}
}
