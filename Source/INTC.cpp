#include <stdio.h>
#include "INTC.h"
#include "DMAC.h"
#include "PS2OS.h"

uint32 CINTC::m_INTC_STAT = 0;
uint32 CINTC::m_INTC_MASK = 0;

using namespace Framework;

void CINTC::Reset()
{
	m_INTC_STAT = 0;
	m_INTC_MASK = 0;
}

void CINTC::CheckInterrupts()
{
	if(CDMAC::IsInterruptPending())
	{
		m_INTC_STAT |= 0x02;
	}

	if((m_INTC_STAT & m_INTC_MASK) != 0)
	{
		//Trigger an interrupt on the EE, but for now, we're gonna call the OS
		CPS2OS::ExceptionHandler();
	}
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
		printf("INTC: Read an unhandled register (0x%0.8X).\r\n", nAddress);
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
		CheckInterrupts();
		break;
	default:
		printf("INTC: Wrote to an unhandled register (0x%0.8X).\r\n", nAddress);
		break;
	}
}

void CINTC::AssertLine(uint32 nLine)
{
	m_INTC_STAT |= (1 << nLine);
	CheckInterrupts();
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
