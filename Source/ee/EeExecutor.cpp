#ifndef _MSC_VER
#include <sys/mman.h>
#endif
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

#ifdef _MSC_VER
	m_handler = AddVectoredExceptionHandler(TRUE, &CEeExecutor::HandleException);
	assert(m_handler != NULL);
#else
	struct sigaction sigAction;
	sigAction.sa_handler	= nullptr;
	sigAction.sa_sigaction	= &HandleException;
	sigAction.sa_flags		= SA_SIGINFO;
	sigemptyset(&sigAction.sa_mask);
	int result = sigaction(SIGSEGV, &sigAction, nullptr);
	assert(result >= 0);
#endif
}

CEeExecutor::~CEeExecutor()
{
#ifdef _MSC_VER
	RemoveVectoredExceptionHandler(m_handler);
#endif
	g_eeExecutor = nullptr;
}

void CEeExecutor::Reset()
{
	SetMemoryProtected(m_ram, PS2::EE_RAM_SIZE, false);
	CMipsExecutor::Reset();
}

void CEeExecutor::ClearActiveBlocksInRange(uint32 start, uint32 end)
{
	uint32 rangeSize = end - start;
	SetMemoryProtected(m_ram + start, rangeSize, false);
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
		SetMemoryProtected(m_ram + start, end - start + 4, true);
	}
	return CMipsExecutor::BlockFactory(context, start, end);
}

void CEeExecutor::SetMemoryProtected(void* addr, size_t size, bool protect)
{
#ifdef _MSC_VER
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(addr, size, protect ? PAGE_READONLY : PAGE_READWRITE, &oldProtect);
	assert(result == TRUE);
#else
	uintptr_t addrValue = reinterpret_cast<uintptr_t>(addr) & ~(m_pageSize - 1);
	addr = reinterpret_cast<void*>(addrValue);
	size = size + (m_pageSize - 1) & ~(m_pageSize - 1);
	int result = mprotect(addr, size, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	assert(result >= 0);
#endif
}

#ifdef _MSC_VER

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

#else

void CEeExecutor::HandleException(int sigId, siginfo_t* sigInfo, void* baseContext)
{
	g_eeExecutor->HandleExceptionInternal(sigId, sigInfo, baseContext);
}

void CEeExecutor::HandleExceptionInternal(int sigId, siginfo_t* sigInfo, void* baseContext)
{
	if(sigId != SIGSEGV) return;
	ptrdiff_t addr = reinterpret_cast<uint8*>(sigInfo->si_addr) - m_ram;
	if(addr >= 0 && addr < PS2::EE_RAM_SIZE)
	{
		addr &= ~(m_pageSize - 1);
		ClearActiveBlocksInRange(addr, addr + m_pageSize);
		return;
	}
	signal(SIGSEGV, SIG_DFL);
}

#endif
