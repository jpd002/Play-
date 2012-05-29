#include <assert.h>
#include "Iop_RootCounters.h"
#include "Iop_Intc.h"
#include "../Log.h"

#define LOG_NAME ("iop_counters")

using namespace Iop;

const uint32 CRootCounters::g_counterInterruptLines[MAX_COUNTERS] =
{
	CIntc::LINE_RTC0,
	CIntc::LINE_RTC1,
	CIntc::LINE_RTC2,
	CIntc::LINE_RTC3,
	CIntc::LINE_RTC4,
	CIntc::LINE_RTC5
};

const uint32 CRootCounters::g_counterBaseAddresses[MAX_COUNTERS] =
{
	CNT0_BASE,
	CNT1_BASE,
	CNT2_BASE,
	CNT3_BASE,
	CNT4_BASE,
	CNT5_BASE
};

const uint32 CRootCounters::g_counterSources[MAX_COUNTERS] =
{
	COUNTER_SOURCE_SYSCLOCK | COUNTER_SOURCE_PIXEL | COUNTER_SOURCE_HOLD,
	COUNTER_SOURCE_SYSCLOCK | COUNTER_SOURCE_HLINE | COUNTER_SOURCE_HOLD,
	COUNTER_SOURCE_SYSCLOCK,
	COUNTER_SOURCE_SYSCLOCK,
	COUNTER_SOURCE_SYSCLOCK,
	COUNTER_SOURCE_SYSCLOCK
};

const uint32 CRootCounters::g_counterSizes[MAX_COUNTERS] =
{
	16,
	16,
	16,
	32,
	32,
	32
};

CRootCounters::CRootCounters(unsigned int clockFreq, Iop::CIntc& intc)
: m_hsyncClocks(clockFreq / (480 * 60))
, m_pixelClocks(clockFreq / (640 * 480 * 60))
, m_intc(intc)
{
	Reset();
}

CRootCounters::~CRootCounters()
{

}

unsigned int CRootCounters::GetCounterIdByAddress(uint32 address)
{
	return (address >= ADDR_BEGIN2) ? ((address - CNT3_BASE) / 0x10) + 3 : ((address - CNT0_BASE) / 0x10);
}

void CRootCounters::Reset()
{
	memset(&m_counter, 0, sizeof(m_counter));
}

void CRootCounters::Update(unsigned int ticks)
{
	for(unsigned int i = 0; i < MAX_COUNTERS; i++)
	{
		COUNTER& counter = m_counter[i];
		if(i == 2 && counter.mode.en) continue;
		//Compute count increment
		unsigned int clockRatio = 1;
		if(i == 0 && counter.mode.clc)
		{
			clockRatio = m_pixelClocks;
		}
		if(i == 1 && counter.mode.clc)
		{
			clockRatio = m_hsyncClocks;
		}
		if(i == 2 && counter.mode.div)
		{
			clockRatio = 8;
		}
		unsigned int totalTicks = counter.clockRemain + ticks;
		unsigned int countAdd = totalTicks / clockRatio;
		counter.clockRemain = totalTicks % clockRatio;
		//Update count
		uint32 counterMax = 0;
		if(g_counterSizes[i] == 16)
		{
			counterMax = counter.mode.tar ? static_cast<uint16>(counter.target) : 0xFFFF;
		}
		else
		{
			counterMax = counter.mode.tar ? counter.target : 0xFFFFFFFF;
		}
		uint32 counterTemp = counter.count + countAdd;
		if(counterTemp >= counterMax)
		{
			counterTemp -= counterMax;
			if(counter.mode.iq1 && counter.mode.iq2)
			{
				m_intc.AssertLine(g_counterInterruptLines[i]);
			}
		}
		if(g_counterSizes[i] == 16)
		{
			counter.count = static_cast<uint16>(counterTemp);
		}
		else
		{
			counter.count = counterTemp;
		}
	}
}

uint32 CRootCounters::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	unsigned int counterId = GetCounterIdByAddress(address);
	unsigned int registerId = address & 0x0F;
	assert(counterId < MAX_COUNTERS);
	switch(registerId)
	{
	case CNT_COUNT:
		return m_counter[counterId].count;
		break;
	case CNT_MODE:
		return m_counter[counterId].mode;
		break;
	case CNT_TARGET:
		return m_counter[counterId].target;
		break;
	}
	return 0;
}

uint32 CRootCounters::WriteRegister(uint32 address, uint32 value)
{
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
	unsigned int counterId = GetCounterIdByAddress(address);
	unsigned int registerId = address & 0x0F;
	assert(counterId < MAX_COUNTERS);
	COUNTER& counter = m_counter[counterId];
	switch(registerId)
	{
	case CNT_COUNT:
		counter.count = value;
		break;
	case CNT_MODE:
		counter.mode <<= value;
		break;
	case CNT_TARGET:
		counter.target = value;
		break;
	}
	return 0;
}

void CRootCounters::DisassembleRead(uint32 address)
{
	unsigned int counterId = GetCounterIdByAddress(address);
	unsigned int registerId = address & 0x0F;
	switch(registerId)
	{
	case CNT_COUNT:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: = COUNT\r\n", counterId);
		break;
	case CNT_MODE:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: = MODE\r\n", counterId);
		break;
	case CNT_TARGET:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: = TARGET\r\n", counterId);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading an unknown register (0x%0.8X).\r\n", address);
		break;
	}
}

void CRootCounters::DisassembleWrite(uint32 address, uint32 value)
{
	unsigned int counterId = GetCounterIdByAddress(address);
	unsigned int registerId = address & 0x0F;
	switch(registerId)
	{
	case CNT_COUNT:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: COUNT = 0x%0.4X\r\n", counterId, value);
		break;
	case CNT_MODE:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: MODE = 0x%0.8X\r\n", counterId, value);
		break;
	case CNT_TARGET:
		CLog::GetInstance().Print(LOG_NAME, "CNT%d: TARGET = 0x%0.4X\r\n", counterId, value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Writing to an unknown register (0x%0.8X, 0x%0.8X).\r\n", address, value);
		break;
	}
}
