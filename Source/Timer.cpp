#include <stdio.h>
#include "Timer.h"
#include "PS2VM.h"
#include "INTC.h"

CTimer::TIMER		CTimer::m_Timer[4];

void CTimer::Reset()
{
	memset(m_Timer, 0, sizeof(TIMER) * 4);
}

void CTimer::Count(unsigned int nCycles)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		TIMER* pTimer;
		uint32 nPrevious, nNext, nCompare;

		pTimer = &m_Timer[i];

		if(!(pTimer->nMODE & 0x80)) continue;

		nPrevious	= pTimer->nCOUNT;
		nNext		= pTimer->nCOUNT;

		switch(pTimer->nMODE & 0x03)
		{
		case 0x00:
		case 0x03:
			nNext += nCycles;
			break;
		case 0x01:
			nNext += nCycles * 16;
			break;
		case 0x02:
			nNext += nCycles * 256;
			break;
		}

		nCompare = (pTimer->nCOMP == 0) ? 0x10000 : pTimer->nCOMP;

		//Check if it hit the reference value
		if((nPrevious < nCompare) && (nNext >= nCompare))
		{
			pTimer->nMODE |= 0x400;
		}

		pTimer->nCOUNT = nNext;
/*
		if(pTimer->nCOUNT >= 0xFFFF)
		{
			pTimer->nMODE |= 0x800;
			pTimer->nCOUNT &= 0xFFFF;
		}
*/
		if(i == 2)
		{
			uint32 nMask;
			nMask = (pTimer->nMODE & 0x300) << 2;
			if((pTimer->nMODE & nMask) != 0)
			{
				CINTC::AssertLine(CINTC::INTC_LINE_TIMER2);
			}
		}
	}
}

uint32 CTimer::GetRegister(uint32 nAddress)
{
	unsigned int nTimerId;

	DisassembleGet(nAddress);

	nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		return m_Timer[nTimerId].nCOUNT & 0xFFFF;
		break;
	case 0x04:
	case 0x08:
	case 0x0C:
		break;

	case 0x10:
		return m_Timer[nTimerId].nMODE;
		break;
	case 0x14:
	case 0x18:
	case 0x1C:
		break;

	case 0x20:
		return m_Timer[nTimerId].nCOMP;
		break;
	case 0x24:
	case 0x28:
	case 0x2C:
		break;

	case 0x30:
		return m_Timer[nTimerId].nHOLD;
		break;
	case 0x34:
	case 0x38:
	case 0x3C:
		break;

	default:
		printf("Timer: Read an unhandled IO port (0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, CPS2VM::m_EE.m_State.nPC);
		break;
	}

	return 0;
}

void CTimer::SetRegister(uint32 nAddress, uint32 nValue)
{
	unsigned int nTimerId;

	DisassembleSet(nAddress, nValue);

	nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		m_Timer[nTimerId].nCOUNT = nValue & 0xFFFF;
		break;
	case 0x04:
	case 0x08:
	case 0x0C:
		break;

	case 0x10:
		m_Timer[nTimerId].nMODE &= ~(nValue & 0xC00);
		m_Timer[nTimerId].nMODE &= 0xC00;
		m_Timer[nTimerId].nMODE |= nValue & ~0xC00;
		break;
	case 0x14:
	case 0x18:
	case 0x1C:
		break;

	case 0x20:
		m_Timer[nTimerId].nCOMP = nValue & 0xFFFF;
		break;
	case 0x24:
	case 0x28:
	case 0x2C:
		break;

	case 0x30:
		m_Timer[nTimerId].nHOLD = nValue & 0xFFFF;
		break;
	case 0x34:
	case 0x38:
	case 0x3C:
		break;

	default:
		printf("Timer: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nValue, CPS2VM::m_EE.m_State.nPC);
		break;
	}
}

void CTimer::DisassembleGet(uint32 nAddress)
{
	unsigned int nTimerId;

	nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		printf("Timer: = T%i_COUNT (PC: 0x%0.8X)\r\n", nTimerId, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x10:
		printf("Timer: = T%i_MODE (PC: 0x%0.8X)\r\n", nTimerId, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x20:
		printf("Timer: = T%i_COMP (PC: 0x%0.8X)\r\n", nTimerId, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x30:
		printf("Timer: = T%i_HOLD (PC: 0x%0.8X)\r\n", nTimerId, CPS2VM::m_EE.m_State.nPC);
		break;
	}
}

void CTimer::DisassembleSet(uint32 nAddress, uint32 nValue)
{
	unsigned int nTimerId;

	nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		printf("Timer: T%i_COUNT = 0x%0.8X (PC: 0x%0.8X)\r\n", nTimerId, nValue, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x10:
		printf("Timer: T%i_MODE = 0x%0.8X (PC: 0x%0.8X)\r\n", nTimerId, nValue, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x20:
		printf("Timer: T%i_COMP = 0x%0.8X (PC: 0x%0.8X)\r\n", nTimerId, nValue, CPS2VM::m_EE.m_State.nPC);
		break;

	case 0x30:
		printf("Timer: T%i_HOLD = 0x%0.8X (PC: 0x%0.8X)\r\n", nTimerId, nValue, CPS2VM::m_EE.m_State.nPC);
		break;
	}
}
