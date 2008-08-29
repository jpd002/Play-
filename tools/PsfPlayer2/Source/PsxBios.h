#ifndef _PSXBIOS_H_
#define _PSXBIOS_H_

#include "MIPS.h"

class CPsxBios
{
public:
							CPsxBios(CMIPS&, uint8*);
	virtual					~CPsxBios();

	void					Reset();
	void					HandleInterrupt();
	void					HandleException();

private:
	template<typename StructType>
	class CStructManager
	{
	public:
		CStructManager(StructType* structBase, uint32 idBase, uint32 structMax) :
		  m_structBase(structBase),
		  m_idBase(idBase),
		  m_structMax(structMax)
		{

		}

		StructType* GetBase() const
		{
			return m_structBase;
		}

		StructType* operator [](uint32 index)
		{
			index -= m_idBase;
			if(index >= m_structMax)
			{
				throw std::exception();
			}
			StructType* structPtr = m_structBase + index;
			if(!structPtr->isValid)
			{
				return NULL;
			}
			return structPtr;
		}

		uint32 Allocate() const
		{
			for(unsigned int i = 0; i < m_structMax; i++)
			{
				if(!m_structBase[i].isValid) 
				{
					m_structBase[i].isValid = true;
					return (i + m_idBase);
				}
			}
			return -1;
		}

		void Free(uint32 id) const
		{
			StructType* structPtr = (*this)[id];
			if(!structPtr[i].isValid)
			{
				throw std::exception();
			}
			structPtr->isValid = false;
		}

	private:
		StructType* m_structBase;
		uint32		m_structMax;
		uint32		m_idBase;
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
	void					sc_bzero();
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

	CStructManager<EVENT>	m_events;

	static SyscallHandler	m_handlerA0[MAX_HANDLER_A0];
	static SyscallHandler	m_handlerB0[MAX_HANDLER_B0];
	static SyscallHandler	m_handlerC0[MAX_HANDLER_C0];
};

#endif
