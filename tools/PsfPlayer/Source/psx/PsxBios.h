#ifndef _PSXBIOS_H_
#define _PSXBIOS_H_

#include "iop/Iop_BiosBase.h"
#include "OsStructManager.h"
#include "MIPS.h"

class CPsxBios : public Iop::CBiosBase
{
public:
							CPsxBios(CMIPS&, uint8*, uint32);
	virtual					~CPsxBios();

	void					Reset();
	void					HandleInterrupt();
	void					HandleException();
	void					CountTicks(uint32);

    void                    LoadExe(uint8*);

    void                    SaveState(Framework::CZipArchiveWriter&);
    void                    LoadState(Framework::CZipArchiveReader&);

	void					NotifyVBlankStart();
	void					NotifyVBlankEnd();

    bool                    IsIdle();

#ifdef DEBUGGER_INCLUDED
	void					LoadDebugTags(Framework::Xml::CNode*);
	void					SaveDebugTags(Framework::Xml::CNode*);
	MipsModuleList			GetModuleList();
#endif

private:
	struct EXEHEADER
	{
		uint8	id[8];
		uint32	text;
		uint32	data;
		uint32	pc0;
		uint32	gp0;
		uint32	textAddr;
		uint32	textSize;
		uint32	dataAddr;
		uint32	dataSize;
		uint32	bssAddr;
		uint32	bssSize;
		uint32	stackAddr;
		uint32	stackSize;
		uint32	savedSp;
		uint32	savedFp;
		uint32	savedGp;
		uint32	savedRa;
		uint32	savedS0;
	};

	struct EVENT
	{
		uint32 isValid;
		uint32 enabled;
		uint32 classId;
		uint32 spec;
		uint32 mode;
		uint32 func;
		uint32 fired;
	};

	enum
	{
		MAX_EVENT = 32,
	};

	enum
	{
		EVENT_ID_RCNT2 = 0xF2000002,
	};

	typedef void			(CPsxBios::*SyscallHandler)();

	void					LongJump(uint32, uint32 = 0);
	uint32&					LongJmpBuffer() const;

	void					SaveCpuState();
	void					LoadCpuState();

	void					DisassembleSyscall(uint32);

	void					ProcessSubFunction(SyscallHandler*, unsigned int);
	void					AssembleInterruptHandler();
	void					AssembleEventChecker();

	//A0
	void					sc_setjmp();
	void					sc_longjmp();
	void					sc_strcpy();
	void					sc_bzero();
	void					sc_memset();
	void					sc_InitHeap();
	void					sc_printf();
	void					sc_FlushCache();
	void					sc_bu_init();
	void					sc_96_remove();
	void					sc_SetMem();

	//B0
	void					sc_SysMalloc();
	void					sc_DeliverEvent();
	void					sc_OpenEvent();
	void					sc_WaitEvent();
	void					sc_TestEvent();
	void					sc_EnableEvent();
	void					sc_DisableEvent();
	void					sc_PAD_dr();
	void					sc_ReturnFromException();
	void					sc_HookEntryInt();
	void					sc_InitCARD();
	void					sc_StartCARD();
	void					sc_GetC0Table();
	void					sc_GetB0Table();
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

	CMIPS&					m_cpu;
	uint8*					m_ram;
    uint32                  m_ramSize;

	COsStructManager<EVENT>	m_events;

	static SyscallHandler	m_handlerA0[MAX_HANDLER_A0];
	static SyscallHandler	m_handlerB0[MAX_HANDLER_B0];
	static SyscallHandler	m_handlerC0[MAX_HANDLER_C0];
};

#endif
