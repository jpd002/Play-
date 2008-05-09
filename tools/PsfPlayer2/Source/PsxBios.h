#ifndef _PSXBIOS_H_
#define _PSXBIOS_H_

#include "MIPS.h"

class CPsxBios
{
public:
							CPsxBios(CMIPS&);
	virtual					~CPsxBios();

	void					Reset();
	void					HandleInterrupt();
	void					HandleException();

private:
	typedef void			(CPsxBios::*SyscallHandler)();

	void					LongJump(uint32);

	void					DisassembleSyscall(uint32);

	void					ProcessSubFunction(SyscallHandler*, unsigned int);

	//A0
	void					sc_InitHeap();
	void					sc_bu_init();
	void					sc_96_remove();
	void					sc_SetMem();

	//B0
	void					sc_SysMalloc();
	void					sc_OpenEvent();
	void					sc_EnableEvent();
	void					sc_ReturnFromException();
	void					sc_HookEntryInt();
	void					sc_ChangeClearPad();

	//C0
	void					sc_SysDeqIntRP();
	void					sc_SysInitMemory();
	void					sc_ChangeClearRCnt();

	void					sc_EnterCriticalSection();
	void					sc_ExitCriticalSection();
	void					sc_Illegal();

	enum
	{
		MAX_HANDLER_A0 = 0x100,
		MAX_HANDLER_B0 = 0x80,
		MAX_HANDLER_C0 = 0x20,
	};

	CMIPS&			m_cpu;
	uint32			m_longJmpBuffer;

	static SyscallHandler	m_handlerA0[MAX_HANDLER_A0];
	static SyscallHandler	m_handlerB0[MAX_HANDLER_B0];
	static SyscallHandler	m_handlerC0[MAX_HANDLER_C0];
};

#endif
