#include "EeExecutor.h"
#include "../Ps2Const.h"
#include "AlignedAlloc.h"

#if defined(__ANDROID__) || defined(__APPLE__)
#include <sys/mman.h>
#endif

#if defined(__APPLE__)

#include <TargetConditionals.h>

#if TARGET_CPU_ARM
#define DISABLE_PROTECTION
#define STATE_FLAVOR			ARM_THREAD_STATE32
#define STATE_FLAVOR_COUNT		ARM_THREAD_STATE32_COUNT
#elif TARGET_CPU_X86
#define STATE_FLAVOR			x86_THREAD_STATE32
#define STATE_FLAVOR_COUNT		x86_THREAD_STATE32_COUNT
#elif TARGET_CPU_X86_64
#define STATE_FLAVOR			x86_THREAD_STATE64
#define STATE_FLAVOR_COUNT		x86_THREAD_STATE64_COUNT
#else
#error Unsupported CPU architecture
#endif

#endif

static CEeExecutor* g_eeExecutor = nullptr;

CEeExecutor::CEeExecutor(CMIPS& context, uint8* ram)
: CMipsExecutor(context, 0x20000000)
, m_ram(ram)
{
	m_pageSize = framework_getpagesize();
}

CEeExecutor::~CEeExecutor()
{

}

void CEeExecutor::AddExceptionHandler()
{
	assert(g_eeExecutor == nullptr);
	g_eeExecutor = this;

#ifdef DISABLE_PROTECTION
	return;
#endif

#if defined(_WIN32)
	m_handler = AddVectoredExceptionHandler(TRUE, &CEeExecutor::HandleException);
	assert(m_handler != NULL);
#elif defined(__ANDROID__)
	struct sigaction sigAction;
	sigAction.sa_handler	= nullptr;
	sigAction.sa_sigaction	= &HandleException;
	sigAction.sa_flags		= SA_SIGINFO;
	sigemptyset(&sigAction.sa_mask);
	int result = sigaction(SIGSEGV, &sigAction, nullptr);
	assert(result >= 0);
#elif defined(__APPLE__)
	kern_return_t result = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &m_port);
	assert(result == KERN_SUCCESS);

	m_handlerThread = std::thread([this] () { HandlerThreadProc(); });

	result = mach_port_insert_right(mach_task_self(), m_port, m_port, MACH_MSG_TYPE_MAKE_SEND);
	assert(result == KERN_SUCCESS);
	
	result = thread_set_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, m_port, EXCEPTION_STATE | MACH_EXCEPTION_CODES, STATE_FLAVOR);
	assert(result == KERN_SUCCESS);

	result = mach_port_mod_refs(mach_task_self(), m_port, MACH_PORT_RIGHT_SEND, -1);
	assert(result == KERN_SUCCESS);
#endif
}

void CEeExecutor::RemoveExceptionHandler()
{
#if defined(_WIN32)
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

bool CEeExecutor::HandleAccessFault(intptr_t ptr)
{
	ptrdiff_t addr = reinterpret_cast<uint8*>(ptr) - m_ram;
	if(addr >= 0 && addr < PS2::EE_RAM_SIZE)
	{
		addr &= ~(m_pageSize - 1);
		ClearActiveBlocksInRange(addr, addr + m_pageSize);
		return true;
	}
	return false;
}

void CEeExecutor::SetMemoryProtected(void* addr, size_t size, bool protect)
{
#ifdef DISABLE_PROTECTION
	return;
#endif

#if defined(_WIN32)
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(addr, size, protect ? PAGE_READONLY : PAGE_READWRITE, &oldProtect);
	assert(result == TRUE);
#elif defined(__ANDROID__) || defined(__APPLE__)
	uintptr_t addrValue = reinterpret_cast<uintptr_t>(addr) & ~(m_pageSize - 1);
	addr = reinterpret_cast<void*>(addrValue);
	size = size + (m_pageSize - 1) & ~(m_pageSize - 1);
	int result = mprotect(addr, size, protect ? PROT_READ : PROT_READ | PROT_WRITE);
	assert(result >= 0);
#else
	assert(false);
#endif
}

#if defined(_WIN32)

LONG WINAPI CEeExecutor::HandleException(_EXCEPTION_POINTERS* exceptionInfo)
{
	return g_eeExecutor->HandleExceptionInternal(exceptionInfo);
}

LONG CEeExecutor::HandleExceptionInternal(_EXCEPTION_POINTERS* exceptionInfo)
{
	auto exceptionRecord = exceptionInfo->ExceptionRecord;
	if(exceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if(HandleAccessFault(exceptionRecord->ExceptionInformation[1]))
		{
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

#elif defined(__ANDROID__)

void CEeExecutor::HandleException(int sigId, siginfo_t* sigInfo, void* baseContext)
{
	g_eeExecutor->HandleExceptionInternal(sigId, sigInfo, baseContext);
}

void CEeExecutor::HandleExceptionInternal(int sigId, siginfo_t* sigInfo, void* baseContext)
{
	if(sigId != SIGSEGV) return;
	if(HandleAccessFault(reinterpret_cast<intptr_t>(sigInfo->si_addr)))
	{
		return;
	}
	signal(SIGSEGV, SIG_DFL);
}

#elif defined(__APPLE__)

void CEeExecutor::HandlerThreadProc()
{
#pragma pack(push, 4)
	struct INPUT_MESSAGE
	{
		mach_msg_header_t head;
		NDR_record_t ndr;
		exception_type_t exception;
		mach_msg_type_number_t codeCount;
		intptr_t code[2];
		int flavor;
		mach_msg_type_number_t stateCount;
		natural_t state[STATE_FLAVOR_COUNT];
		mach_msg_trailer_t trailer;
	};
	struct OUTPUT_MESSAGE
	{
		mach_msg_header_t head;
		NDR_record_t ndr;
		kern_return_t result;
		int flavor;
		mach_msg_type_number_t stateCount;
		natural_t state[STATE_FLAVOR_COUNT];
	};
#pragma pack(pop)

	while(1)
	{
		kern_return_t result = KERN_SUCCESS;
		
		INPUT_MESSAGE inMsg;
		result = mach_msg(&inMsg.head, MACH_RCV_MSG | MACH_RCV_LARGE, 0, sizeof(inMsg), m_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		assert(result == KERN_SUCCESS);
		
		assert(inMsg.head.msgh_id == 2406);	//MACH_EXCEPTION_RAISE_RPC
		
		bool success = HandleAccessFault(inMsg.code[1]);
		
		OUTPUT_MESSAGE outMsg;
		outMsg.head.msgh_bits			= MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(inMsg.head.msgh_bits), 0);
		outMsg.head.msgh_remote_port	= inMsg.head.msgh_remote_port;
		outMsg.head.msgh_local_port		= MACH_PORT_NULL;
		outMsg.head.msgh_id				= inMsg.head.msgh_id + 100;
		outMsg.head.msgh_size			= sizeof(outMsg);
		outMsg.ndr						= inMsg.ndr;

		if(success)
		{
			outMsg.result		= KERN_SUCCESS;
			outMsg.flavor		= STATE_FLAVOR;
			outMsg.stateCount	= STATE_FLAVOR_COUNT;
			memcpy(outMsg.state, inMsg.state, STATE_FLAVOR_COUNT * sizeof(natural_t));
		}
		else
		{
			outMsg.result		= KERN_FAILURE;
			outMsg.flavor		= 0;
			outMsg.stateCount	= 0;
		}
		
		result = mach_msg(&outMsg.head, MACH_SEND_MSG | MACH_RCV_LARGE, sizeof(outMsg), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		assert(result == KERN_SUCCESS);
	}
}

#endif
