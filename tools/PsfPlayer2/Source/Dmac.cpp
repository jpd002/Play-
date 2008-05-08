#include "Dmac.h"
#include "Log.h"

#define LOG_NAME ("psx_dmac")

using namespace Psx;

CDmac::CDmac(uint8* ram) :
m_ram(ram)
{
	Reset();
}

CDmac::~CDmac()
{

}

void CDmac::Reset()
{
	m_DPCR = 0;
	m_DICR = 0;
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
			m_DICR = value;
			break;
		}
	}
	else
	{

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
		case CH_MADR:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : MADR = 0x%0.8X.\r\n",
				channel, value);
			break;
		case CH_BCR:
			CLog::GetInstance().Print(LOG_NAME, "CH%i : BCR = 0x%0.8X.\r\n",
				channel, value);
			break;
		case CH_CHCR:
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
