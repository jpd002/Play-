#include "Dmac.h"
#include "Intc.h"
#include "Log.h"

#define LOG_NAME ("psx_dmac")

using namespace Psx;
using namespace std;

CDmac::CDmac(uint8* ram, CIntc& intc) :
m_ram(ram),
m_intc(intc),
m_channelSpu(CH4_BASE, 4, *this)
{
	memset(m_channel, 0, sizeof(m_channel));
	m_channel[4] = &m_channelSpu;
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
		CDmaChannel* channel(m_channel[i]);
		if(channel == NULL) continue;
		channel->Reset();
	}
}

void CDmac::SetReceiveFunction(unsigned int channelNumber, const CDmaChannel::ReceiveFunctionType& handler)
{
	if(channelNumber >= MAX_CHANNEL)
	{
		throw runtime_error("Invalid channel number.");
	}
	CDmaChannel* channel(m_channel[channelNumber]);
	if(channel != NULL)
	{
		channel->SetReceiveFunction(handler);
	}
}

uint8* CDmac::GetRam() const
{
	return m_ram;
}

void CDmac::AssertLine(unsigned int line)
{
	m_DICR |= 1 << (line + 24);
	m_intc.AssertLine(CIntc::LINE_DMAC);
}

uint32 CDmac::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	if(address >= GENERAL_REGS_BASE)
	{
		switch(address)
		{
		case DPCR:
			return m_DPCR;
			break;
		case DICR:
			return m_DICR;
			break;
		}
	}
	else
	{
		CDmaChannel* channel(m_channel[(address - CH0_BASE) / 0x10]);
		if(channel != NULL)
		{
			return channel->ReadRegister(address);
		}
	}
	return 0;
}

void CDmac::WriteRegister(uint32 address, uint32 value)
{
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
	if(address >= GENERAL_REGS_BASE)
	{
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
		}
	}
	else
	{
		CDmaChannel* channel(m_channel[(address - CH0_BASE) / 0x10]);
		if(channel != NULL)
		{
			channel->WriteRegister(address, value);
		}
	}
}

void CDmac::DisassembleRead(uint32 address)
{
	if(address >= GENERAL_REGS_BASE)
	{
		switch(address)
		{
		case DPCR:
			CLog::GetInstance().Print(LOG_NAME, "= DPCR\r\n");
			break;
		case DICR:
			CLog::GetInstance().Print(LOG_NAME, "= DICR\r\n");
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Read an unknown register. (0x%0.8X).\r\n", address);
			break;
		}
	}
	else
	{
		unsigned int channel = (address - CH0_BASE) / 0x10;
		unsigned int registerId = address & 0x0F;
		switch(registerId)
		{
		default:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : Read an unknown register. (0x%0.8X).\r\n",
				channel, registerId);
			break;
		}
	}
}

void CDmac::DisassembleWrite(uint32 address, uint32 value)
{
	if(address >= GENERAL_REGS_BASE)
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
			CLog::GetInstance().Print(LOG_NAME, "Wrote an unknown register. (0x%0.8X, 0x%0.8X).\r\n", address, value);
			break;
		}
	}
	else
	{
		unsigned int channel = (address - CH0_BASE) / 0x10;
		unsigned int registerId = address & 0x0F;
		switch(registerId)
		{
		case CDmaChannel::REG_MADR:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : MADR = 0x%0.8X.\r\n",
				channel, value);
			break;
		case CDmaChannel::REG_BCR:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : BCR = 0x%0.8X.\r\n",
				channel, value);
			break;
		case CDmaChannel::REG_CHCR:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : CHCR = 0x%0.8X.\r\n",
				channel, value);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : Wrote an unknown register. (0x%0.8X, 0x%0.8X).\r\n",
				channel, registerId, value);
			break;
		}
	}
}
