#include <stdio.h>
#include "Timer.h"
#include "Log.h"

#define LOG_NAME ("timer")

CTimer::CTimer(CINTC& intc) 
: m_intc(intc)
{
	Reset();
}

CTimer::~CTimer()
{

}

void CTimer::Reset()
{
	memset(m_timer, 0, sizeof(TIMER) * 4);
}

void CTimer::Count(unsigned int ticks)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		TIMER* timer = &m_timer[i];

		if(!(timer->nMODE & MODE_COUNT_ENABLE)) continue;

		uint32 previousCount	= timer->nCOUNT;
		uint32 nextCount		= timer->nCOUNT;

		uint32 divider = 1;
		switch(timer->nMODE & 0x03)
		{
		case 0x00:
		case 0x03:
			divider = 1;
			break;
		case 0x01:
			divider = 16;
			break;
		case 0x02:
			divider = 256;
			break;
		}

		//Compute increment
		uint32 totalTicks = timer->clockRemain + ticks;
		uint32 countAdd = totalTicks / divider;
		timer->clockRemain = totalTicks % divider;
		nextCount = previousCount + countAdd;

		uint32 compare = (timer->nCOMP == 0) ? 0x10000 : timer->nCOMP;

		//Check if it hit the reference value
		if((previousCount < compare) && (nextCount >= compare))
		{
			timer->nMODE |= MODE_EQUAL_FLAG;
			if(timer->nMODE & MODE_ZERO_RETURN)
			{
				timer->nCOUNT = nextCount - compare;
			}
			else
			{
				timer->nCOUNT = nextCount;
			}
		}
		else
		{
			timer->nCOUNT = nextCount;
		}

		if(timer->nCOUNT >= 0xFFFF)
		{
			timer->nMODE |= MODE_OVERFLOW_FLAG;
			timer->nCOUNT &= 0xFFFF;
		}

		uint32 nMask = (timer->nMODE & 0x300) << 2;
		bool interruptPending = (timer->nMODE & nMask) != 0;
		if(interruptPending)
		{
			if(i == 1)
			{
				m_intc.AssertLine(CINTC::INTC_LINE_TIMER1);
			}
			else if(i == 2)
			{
				m_intc.AssertLine(CINTC::INTC_LINE_TIMER2);
			}
		}
	}
}

uint32 CTimer::GetRegister(uint32 nAddress)
{
	DisassembleGet(nAddress);

	unsigned int nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		return m_timer[nTimerId].nCOUNT & 0xFFFF;
		break;
	case 0x04:
	case 0x08:
	case 0x0C:
		break;

	case 0x10:
		return m_timer[nTimerId].nMODE;
		break;
	case 0x14:
	case 0x18:
	case 0x1C:
		break;

	case 0x20:
		return m_timer[nTimerId].nCOMP;
		break;
	case 0x24:
	case 0x28:
	case 0x2C:
		break;

	case 0x30:
		return m_timer[nTimerId].nHOLD;
		break;
	case 0x34:
	case 0x38:
	case 0x3C:
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled IO port (0x%0.8X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CTimer::SetRegister(uint32 nAddress, uint32 nValue)
{
	DisassembleSet(nAddress, nValue);

	unsigned int nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		m_timer[nTimerId].nCOUNT = nValue & 0xFFFF;
		break;
	case 0x04:
	case 0x08:
	case 0x0C:
		break;

	case 0x10:
		m_timer[nTimerId].nMODE &= ~(nValue & 0xC00);
		m_timer[nTimerId].nMODE &= 0xC00;
		m_timer[nTimerId].nMODE |= nValue & ~0xC00;
		break;
	case 0x14:
	case 0x18:
	case 0x1C:
		break;

	case 0x20:
		m_timer[nTimerId].nCOMP = nValue & 0xFFFF;
		break;
	case 0x24:
	case 0x28:
	case 0x2C:
		break;

	case 0x30:
		m_timer[nTimerId].nHOLD = nValue & 0xFFFF;
		break;
	case 0x34:
	case 0x38:
	case 0x3C:
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X).\r\n", nAddress, nValue);
		break;
	}
}

void CTimer::DisassembleGet(uint32 nAddress)
{
	unsigned int nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		CLog::GetInstance().Print(LOG_NAME, "= T%i_COUNT\r\n", nTimerId);
		break;

	case 0x10:
		CLog::GetInstance().Print(LOG_NAME, "= T%i_MODE\r\n", nTimerId);
		break;

	case 0x20:
		CLog::GetInstance().Print(LOG_NAME, "= T%i_COMP\r\n", nTimerId);
		break;

	case 0x30:
		CLog::GetInstance().Print(LOG_NAME, "= T%i_HOLD\r\n", nTimerId);
		break;
	}
}

void CTimer::DisassembleSet(uint32 nAddress, uint32 nValue)
{
	unsigned int nTimerId = (nAddress >> 11) & 0x3;

	switch(nAddress & 0x7FF)
	{
	case 0x00:
		CLog::GetInstance().Print(LOG_NAME, "T%i_COUNT = 0x%0.8X\r\n", nTimerId, nValue);
		break;

	case 0x10:
		CLog::GetInstance().Print(LOG_NAME, "T%i_MODE = 0x%0.8X\r\n", nTimerId, nValue);
		break;

	case 0x20:
		CLog::GetInstance().Print(LOG_NAME, "T%i_COMP = 0x%0.8X\r\n", nTimerId, nValue);
		break;

	case 0x30:
		CLog::GetInstance().Print(LOG_NAME, "T%i_HOLD = 0x%0.8X\r\n", nTimerId, nValue);
		break;
	}
}
