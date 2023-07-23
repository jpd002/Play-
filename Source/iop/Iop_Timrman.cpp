#include <assert.h>
#include "Iop_Timrman.h"
#include "Iop_Intc.h"
#include "../Log.h"
#include "IopBios.h"
#include "Iop_RootCounters.h"

#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "states/RegisterStateFile.h"

#define LOG_NAME ("iop_timrman")

#define FUNCTION_ALLOCHARDTIMER "AllocHardTimer"
#define FUNCTION_REFERHARDTIMER "ReferHardTimer"
#define FUNCTION_FREEHARDTIMER "FreeHardTimer"
#define FUNCTION_SETTIMERMODE "SetTimerMode"
#define FUNCTION_GETTIMERSTATUS "GetTimerStatus"
#define FUNCTION_GETTIMERCOUNTER "GetTimerCounter"
#define FUNCTION_SETTIMERCOMPARE "SetTimerCompare"
#define FUNCTION_GETHARDTIMERINTRCODE "GetHardTimerIntrCode"
#define FUNCTION_SETTIMERCALLBACK "SetTimerCallback"
#define FUNCTION_SETOVERFLOWCALLBACK "SetOverflowCallback"
#define FUNCTION_SETUPHARDTIMER "SetupHardTimer"
#define FUNCTION_STARTHARDTIMER "StartHardTimer"
#define FUNCTION_STOPHARDTIMER "StopHardTimer"

#define STATE_FILENAME ("iop_timrman/state.xml")

#define STATE_HARDTIMERALLOC ("HardTimerAlloc")

using namespace Iop;

CTimrman::CTimrman(CIopBios& bios)
    : m_bios(bios)
{
	static_assert((sizeof(m_hardTimerAlloc) * 8) >= Iop::CRootCounters::MAX_COUNTERS);
}

std::string CTimrman::GetId() const
{
	return "timrman";
}

std::string CTimrman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_ALLOCHARDTIMER;
		break;
	case 5:
		return FUNCTION_REFERHARDTIMER;
		break;
	case 6:
		return FUNCTION_FREEHARDTIMER;
		break;
	case 7:
		return FUNCTION_SETTIMERMODE;
		break;
	case 8:
		return FUNCTION_GETTIMERSTATUS;
		break;
	case 10:
		return FUNCTION_GETTIMERCOUNTER;
		break;
	case 11:
		return FUNCTION_SETTIMERCOMPARE;
		break;
	case 16:
		return FUNCTION_GETHARDTIMERINTRCODE;
		break;
	case 20:
		return FUNCTION_SETTIMERCALLBACK;
		break;
	case 21:
		return FUNCTION_SETOVERFLOWCALLBACK;
		break;
	case 22:
		return FUNCTION_SETUPHARDTIMER;
		break;
	case 23:
		return FUNCTION_STARTHARDTIMER;
		break;
	case 24:
		return FUNCTION_STOPHARDTIMER;
		break;
	default:
		return "unknown";
		break;
	}
}

void CTimrman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = AllocHardTimer(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = ReferHardTimer(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = FreeHardTimer(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 7:
		SetTimerMode(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetTimerStatus(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 10:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetTimerCounter(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 11:
		SetTimerCompare(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 16:
		context.m_State.nGPR[CMIPS::V0].nD0 = GetHardTimerIntrCode(
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = SetTimerCallback(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 21:
		context.m_State.nGPR[CMIPS::V0].nD0 = SetOverflowCallback(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 22:
		context.m_State.nGPR[CMIPS::V0].nD0 = SetupHardTimer(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 23:
		context.m_State.nGPR[CMIPS::V0].nD0 = StartHardTimer(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 24:
		context.m_State.nGPR[CMIPS::V0].nD0 = StopHardTimer(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "(%08X): Unknown function (%d) called.\r\n",
		                         context.m_State.nPC, functionId);
		break;
	}
}

void CTimrman::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_FILENAME);
	registerFile->SetRegister32(STATE_HARDTIMERALLOC, m_hardTimerAlloc);
	archive.InsertFile(std::move(registerFile));
}

void CTimrman::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_FILENAME));
	m_hardTimerAlloc = registerFile.GetRegister32(STATE_HARDTIMERALLOC);
}

int32 CTimrman::AllocHardTimer(uint32 source, uint32 size, uint32 prescale)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCHARDTIMER "(source = %d, size = %d, prescale = %d);\r\n",
	                          source, size, prescale);
#endif
	assert(
	    (source == CRootCounters::COUNTER_SOURCE_SYSCLOCK) ||
	    (source == CRootCounters::COUNTER_SOURCE_PIXEL) ||
	    (source == CRootCounters::COUNTER_SOURCE_HLINE));
	//Look for a match in reverse order because counters with
	//multiple sources appear at the beginning of the list
	for(int i = (CRootCounters::MAX_COUNTERS - 1); i >= 0; i--)
	{
		if(
		    (CRootCounters::g_counterSizes[i] == size) &&
		    ((CRootCounters::g_counterSources[i] & source) != 0) &&
		    (CRootCounters::g_counterMaxScales[i] >= prescale) &&
		    ((m_hardTimerAlloc & (1 << i)) == 0))
		{
			m_hardTimerAlloc |= (1 << i);
			return (i + 1);
		}
	}
	CLog::GetInstance().Warn(LOG_NAME, "Couldn't allocate a timer.\r\n");
	return CIopBios::KERNEL_RESULT_ERROR_NO_TIMER;
}

int CTimrman::ReferHardTimer(uint32 source, uint32 size, uint32 mode, uint32 modeMask)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REFERHARDTIMER "(source = %d, size = %d, mode = 0x%08X, mask = 0x%08X);\r\n",
	                          source, size, mode, modeMask);
#endif
	return 0;
}

int32 CTimrman::FreeHardTimer(uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_FREEHARDTIMER "(timerId = %d);\r\n", timerId);
#endif
	uint32 hardTimerIndex = timerId - 1;
	if(hardTimerIndex >= CRootCounters::MAX_COUNTERS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to free an invalid timer id (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	if((m_hardTimerAlloc & (1 << hardTimerIndex)) == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to free a free timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	m_hardTimerAlloc &= ~(1 << hardTimerIndex);
	return 0;
}

void CTimrman::SetTimerMode(CMIPS& context, uint32 timerId, uint32 mode)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERMODE "(timerId = %d, mode = 0x%08X);\r\n",
	                          timerId, mode);
#endif
	if(timerId == 0) return;
	timerId--;
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_MODE, mode);
}

int CTimrman::GetTimerStatus(CMIPS& context, uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETTIMERSTATUS "(timerId = %d).\r\n",
	                          timerId);
#endif
	if(timerId == 0) return 0;
	timerId--;
	return context.m_pMemoryMap->GetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_MODE) | 0x800;
}

int CTimrman::GetTimerCounter(CMIPS& context, uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETTIMERCOUNTER "(timerId = %d).\r\n",
	                          timerId);
#endif
	if(timerId == 0) return 0;
	timerId--;
	return context.m_pMemoryMap->GetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_COUNT);
}

void CTimrman::SetTimerCompare(CMIPS& context, uint32 timerId, uint32 compare)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERCOMPARE "(timerId = %d, compare = 0x%08X);\r\n",
	                          timerId, compare);
#endif
	if(timerId == 0) return;
	timerId--;
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_COUNT, 0);
	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[timerId] + CRootCounters::CNT_TARGET, compare);
}

int CTimrman::GetHardTimerIntrCode(uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETHARDTIMERINTRCODE "(timerId = %d).\r\n",
	                          timerId);
#endif
	if(timerId == 0) return CIntc::LINE_RTC0;
	timerId--;
	return CRootCounters::g_counterInterruptLines[timerId];
}

int32 CTimrman::SetTimerCallback(CMIPS& context, uint32 timerId, uint32 target, uint32 handler, uint32 arg)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTIMERCALLBACK "(timerId = %d, target = %d, handler = 0x%08X, arg = 0x%08X);\r\n",
	                          timerId, target, handler, arg);
#endif
	uint32 hardTimerIndex = timerId - 1;
	if(hardTimerIndex >= CRootCounters::MAX_COUNTERS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Setting callback on an invalid timer id (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	if((m_hardTimerAlloc & (1 << hardTimerIndex)) == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Setting callback on a free timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}

	context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_TARGET, target);

	uint32 timerInterruptLine = CRootCounters::g_counterInterruptLines[hardTimerIndex];
	m_bios.ReleaseIntrHandler(timerInterruptLine);
	m_bios.RegisterIntrHandler(timerInterruptLine, 0, handler, arg);

	return 0;
}

int32 CTimrman::SetOverflowCallback(CMIPS& context, uint32 timerId, uint32 handler, uint32 arg)
{
	uint32 hardTimerIndex = timerId - 1;
	//Our timer hardware implementation doesn't really differentiate overflow & target interrupts.
	//Just use the maximum value of the timer as a target to simulate overflow handling.
	//This would break if there were handlers set for both target and overflow on the same timer.
	uint32 overflowValue = (CRootCounters::g_counterSizes[hardTimerIndex] == 16) ? 0xFFFF : 0xFFFFFFFF;
	return SetTimerCallback(context, timerId, overflowValue, handler, arg);
}

int32 CTimrman::SetupHardTimer(CMIPS& context, uint32 timerId, uint32 source, uint32 gateMode, uint32 prescale)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETUPHARDTIMER "(timerId = %d, source = %d, mode = %d, prescale = %d);\r\n",
	                          timerId, source, gateMode, prescale);
#endif

	uint32 hardTimerIndex = timerId - 1;
	if(hardTimerIndex >= CRootCounters::MAX_COUNTERS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to setup an invalid timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	if((m_hardTimerAlloc & (1 << hardTimerIndex)) == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to setup a free timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}

	assert(gateMode == 0);
	assert((CRootCounters::g_counterSources[hardTimerIndex] & source) != 0);
	assert(CRootCounters::g_counterMaxScales[hardTimerIndex] >= prescale);

	//Set proper clock divider
	auto modeAddr = CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_MODE;
	auto mode = make_convertible<CRootCounters::MODE>(context.m_pMemoryMap->GetWord(modeAddr));
	mode.clc = (source != CRootCounters::COUNTER_SOURCE_SYSCLOCK) ? 1 : 0;

	if(prescale == 1)
		mode.div = CRootCounters::COUNTER_SCALE_1;
	else if(prescale == 8)
		mode.div = CRootCounters::COUNTER_SCALE_8;
	else if(prescale == 16)
		mode.div = CRootCounters::COUNTER_SCALE_16;
	else if(prescale == 256)
		mode.div = CRootCounters::COUNTER_SCALE_256;
	else
		assert(false);

	context.m_pMemoryMap->SetWord(modeAddr, mode);

	return 0;
}

int32 CTimrman::StartHardTimer(CMIPS& context, uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_STARTHARDTIMER "(timerId = %d);\r\n",
	                          timerId);
#endif

	uint32 hardTimerIndex = timerId - 1;
	if(hardTimerIndex >= CRootCounters::MAX_COUNTERS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to start an invalid timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	if((m_hardTimerAlloc & (1 << hardTimerIndex)) == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to start a free timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}

	//Enable timer
	{
		auto modeAddr = CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_MODE;
		auto mode = make_convertible<CRootCounters::MODE>(context.m_pMemoryMap->GetWord(modeAddr));

		mode.tar = 1;
		mode.iq1 = 1;
		mode.iq2 = 1;

		context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_COUNT, 0x0000);
		context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_MODE, mode);
	}

	//Enable interrupts
	{
		uint32 timerInterruptLine = CRootCounters::g_counterInterruptLines[hardTimerIndex];
		if(m_bios.FindIntrHandler(timerInterruptLine) != -1)
		{
			uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
			mask |= (1 << timerInterruptLine);
			context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);
		}
	}

	return 0;
}

int32 CTimrman::StopHardTimer(CMIPS& context, uint32 timerId)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_STOPHARDTIMER "(timerId = %d);\r\n",
	                          timerId);
#endif

	uint32 hardTimerIndex = timerId - 1;
	if(hardTimerIndex >= CRootCounters::MAX_COUNTERS)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to stop an invalid timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}
	if((m_hardTimerAlloc & (1 << hardTimerIndex)) == 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Trying to stop a free timer (%d).\r\n", timerId);
		return CIopBios::KERNEL_RESULT_ERROR_ILLEGAL_TIMERID;
	}

	//Stop timer
	{
		auto modeAddr = CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_MODE;
		auto mode = make_convertible<CRootCounters::MODE>(context.m_pMemoryMap->GetWord(modeAddr));

		mode.tar = 0;
		mode.iq1 = 0;
		mode.iq2 = 0;

		context.m_pMemoryMap->SetWord(CRootCounters::g_counterBaseAddresses[hardTimerIndex] + CRootCounters::CNT_MODE, mode);
	}

	//Disable interrupts
	{
		uint32 timerInterruptLine = CRootCounters::g_counterInterruptLines[hardTimerIndex];
		uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
		mask &= ~(1 << timerInterruptLine);
		context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);
	}

	return 0;
}
