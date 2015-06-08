#include "EeExecutor.h"
#include "../Ps2Const.h"
#include "AlignedAlloc.h"

static CEeExecutor* g_eeExecutor = nullptr;

CEeExecutor::CEeExecutor(CMIPS& context, uint8* ram)
: CMipsExecutor(context, 0x20000000)
, m_ram(ram)
{
	m_pageSize = framework_getpagesize();

	assert(g_eeExecutor == nullptr);
	g_eeExecutor = this;
	m_handler = AddVectoredExceptionHandler(TRUE, &CEeExecutor::HandleException);
	assert(m_handler != NULL);
}

CEeExecutor::~CEeExecutor()
{
	RemoveVectoredExceptionHandler(m_handler);
	g_eeExecutor = nullptr;
}

void CEeExecutor::Reset()
{
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_ram, PS2::EE_RAM_SIZE, PAGE_READWRITE, &oldProtect);
	assert(result == TRUE);
	CMipsExecutor::Reset();
}

void CEeExecutor::ClearActiveBlocksInRange(uint32 start, uint32 end)
{
	uint32 rangeSize = end - start;
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_ram + start, rangeSize, PAGE_READWRITE, &oldProtect);
	assert(result == TRUE);
	CBasicBlock* currentBlock = nullptr;
	if(m_context.m_State.nPC >= start && m_context.m_State.nPC < end)
	{
		//We are currently executing a block in that range, we need to protect it
		//We assume that we're not writing in the same place as the currently executed block
		currentBlock = FindBlockStartingAt(m_context.m_State.nPC);
		assert(currentBlock != nullptr);
	}
	ClearActiveBlocksInRangeInternal(start, end, currentBlock);
}

CMipsExecutor::BasicBlockPtr CEeExecutor::BlockFactory(CMIPS& context, uint32 start, uint32 end)
{
	//Kernel area is below 0x100000 and isn't protected. Some games will write code in there
	//but it is safe to assume that it won't change (code writes some data just besides itself
	//so it keeps generating exceptions, making the game slower)
	if(start >= 0x100000 && start < PS2::EE_RAM_SIZE)
	{
		DWORD oldProtect = 0;
		BOOL result = VirtualProtect(m_ram + start, end - start + 4, PAGE_READONLY, &oldProtect);
		assert(result == TRUE);
	}
	return CMipsExecutor::BlockFactory(context, start, end);
}

LONG WINAPI CEeExecutor::HandleException(_EXCEPTION_POINTERS* exceptionInfo)
{
	return g_eeExecutor->HandleExceptionInternal(exceptionInfo);
}

LONG CEeExecutor::HandleExceptionInternal(_EXCEPTION_POINTERS* exceptionInfo)
{
	auto exceptionRecord = exceptionInfo->ExceptionRecord;
	if(exceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		ptrdiff_t addr = reinterpret_cast<uint8*>(exceptionRecord->ExceptionInformation[1]) - m_ram;
		if(addr >= 0 && addr < PS2::EE_RAM_SIZE)
		{
			addr &= ~(m_pageSize - 1);
			ClearActiveBlocksInRange(addr, addr + m_pageSize);
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
