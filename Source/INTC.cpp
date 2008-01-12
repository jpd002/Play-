#include "INTC.h"
#include "Log.h"

#define LOG_NAME "intc"

using namespace Framework;

CINTC::CINTC(CDMAC& dmac) :
m_INTC_STAT(0),
m_INTC_MASK(0),
m_dmac(dmac)
{

}

CINTC::~CINTC()
{

}

void CINTC::Reset()
{
	m_INTC_STAT = 0;
	m_INTC_MASK = 0;
}

bool CINTC::IsInterruptPending()
{
	if(m_dmac.IsInterruptPending())
	{
		m_INTC_STAT |= 0x02;
	}

	return (m_INTC_STAT & m_INTC_MASK) != 0;
}

uint32 CINTC::GetRegister(uint32 nAddress)
{
	switch(nAddress)
	{
	case 0x1000F000:
		return m_INTC_STAT;
		break;
	case 0x1000F010:
		return m_INTC_MASK;
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled register (0x%0.8X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CINTC::SetRegister(uint32 nAddress, uint32 nValue)
{
	switch(nAddress)
	{
	case 0x1000F000:
		m_INTC_STAT &= ~nValue;
		break;
	case 0x1000F010:
		m_INTC_MASK ^= nValue;
		break;
	default:
        CLog::GetInstance().Print(LOG_NAME, "Wrote to an unhandled register (0x%0.8X).\r\n", nAddress);
		break;
	}
}

void CINTC::AssertLine(uint32 nLine)
{
	m_INTC_STAT |= (1 << nLine);
}

void CINTC::LoadState(CStream* pStream)
{
	pStream->Read(&m_INTC_STAT, 4);
	pStream->Read(&m_INTC_MASK, 4);
}

void CINTC::SaveState(CStream* pStream)
{
	pStream->Write(&m_INTC_STAT, 4);
	pStream->Write(&m_INTC_MASK, 4);
}
