#include <assert.h>
#include "Iop_RootCounters.h"
#include "Iop_Intc.h"
#include "Log.h"

#define LOG_NAME ("iop_counters")

using namespace Iop;
using namespace std;

CRootCounters::CRootCounters(unsigned int clockFreq, Iop::CIntc& intc) :
m_clockFreq(clockFreq),
m_intc(intc)
{
	Reset();
}

CRootCounters::~CRootCounters()
{

}

void CRootCounters::Reset()
{
	memset(&m_counter, 0, sizeof(m_counter));
	for(unsigned int i = 0; i < MAX_COUNTERS; i++)
	{
		m_counter[i].clockRatio = 8;
	}
	m_counter[1].clockRatio = 8;
}

void CRootCounters::Update(unsigned int ticks)
{
	for(unsigned int i = 0; i < MAX_COUNTERS; i++)
	{
		COUNTER& counter = m_counter[i];
		if(i == 2 && counter.mode.en) continue;
		//Compute count increment
		unsigned int totalTicks = counter.clockRemain + ticks;
		unsigned int countAdd = totalTicks / counter.clockRatio;
		counter.clockRemain = totalTicks % counter.clockRatio;
		//Update count
		uint32 counterMax = counter.mode.tar ? counter.target : 0xFFFF;
		uint32 counterTemp = counter.count + countAdd;
		if(counterTemp >= counterMax)
		{
			counterTemp -= counterMax;
			if(counter.mode.iq1 && counter.mode.iq2)
			{
				m_intc.AssertLine(Iop::CIntc::LINE_RTC0 + i);
			}
		}
		counter.count = static_cast<uint16>(counterTemp);
	}
}

uint32 CRootCounters::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	unsigned int counterId = (address - CNT0_BASE) / 0x10;
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
	unsigned int counterId = (address - CNT0_BASE) / 0x10;
	unsigned int registerId = address & 0x0F;
	assert(counterId < MAX_COUNTERS);
	COUNTER& counter = m_counter[counterId];
	switch(registerId)
	{
	case CNT_COUNT:
		counter.count = static_cast<uint16>(value);
		break;
	case CNT_MODE:
		counter.mode <<= value;
		break;
	case CNT_TARGET:
		counter.target = static_cast<uint16>(value);
		break;
	}
	return 0;
}

void CRootCounters::DisassembleRead(uint32 address)
{
	unsigned int counterId = (address - CNT0_BASE) / 0x10;
	unsigned int registerId = address & 0x0F;
	switch(registerId)
	{
	case CNT_COUNT:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: = COUNT\r\n", counterId);
		break;
	case CNT_MODE:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: = MODE\r\n", counterId);
		break;
	case CNT_TARGET:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: = TARGET\r\n", counterId);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading an unknown register (0x%0.8X).\r\n", address);
		break;
	}
}

void CRootCounters::DisassembleWrite(uint32 address, uint32 value)
{
	unsigned int counterId = (address - CNT0_BASE) / 0x10;
	unsigned int registerId = address & 0x0F;
	switch(registerId)
	{
	case CNT_COUNT:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: COUNT = 0x%0.4X\r\n", counterId, value);
		break;
	case CNT_MODE:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: MODE = 0x%0.8X\r\n", counterId, value);
		break;
	case CNT_TARGET:
		CLog::GetInstance().Print(LOG_NAME, "CNT%i: TARGET = 0x%0.4X\r\n", counterId, value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Writing to an unknown register (0x%0.8X, 0x%0.8X).\r\n", address, value);
		break;
	}
}
