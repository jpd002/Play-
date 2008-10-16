#include "PsxBios.h"
#include "PsxVm.h"
#include "COP_SCU.h"
#include "Log.h"
#include "Intc.h"
#include "MipsAssembler.h"

#define LOG_NAME	("psxbios")
#define SC_PARAM0	(CMIPS::A0)
#define SC_PARAM1	(CMIPS::A1)
#define SC_PARAM2	(CMIPS::A2)
#define SC_PARAM3	(CMIPS::A3)
#define SC_RETURN	(CMIPS::V0)

using namespace std;
using namespace Psx;

#define LONGJMP_BUFFER				(0x0200)
#define INTR_HANDLER				(0x1000)
#define EVENT_CHECKER				(0x1200)
#define KERNEL_STACK				(0x2000)
#define EVENTS_BEGIN				(0x3000)
#define EVENTS_SIZE					(sizeof(CPsxBios::EVENT) * CPsxBios::MAX_EVENT)
#define B0TABLE_BEGIN				(EVENTS_BEGIN + EVENTS_SIZE)
#define B0TABLE_SIZE				(0x5D * 4)
#define C0TABLE_BEGIN				(B0TABLE_BEGIN + B0TABLE_SIZE)
#define C0TABLE_SIZE				(0x1C * 4)
#define C0_EXCEPTIONHANDLER_BEGIN	(C0TABLE_BEGIN + C0TABLE_SIZE)
#define C0_EXCEPTIONHANDLER_SIZE	(0x1000)

CPsxBios::CPsxBios(CMIPS& cpu, uint8* ram) :
m_cpu(cpu),
m_ram(ram),
m_events(reinterpret_cast<EVENT*>(&m_ram[EVENTS_BEGIN]), 1, MAX_EVENT)
{

}

CPsxBios::~CPsxBios()
{

}

void CPsxBios::Reset()
{
	uint32 syscallAddress[3] = { 0xA0, 0xB0, 0xC0 };

	for(unsigned int i = 0; i < 3; i++)
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + syscallAddress[i]));
		assembler.SYSCALL();
		assembler.JR(CMIPS::RA);
	}

	//Setup kernel stack
	m_cpu.m_State.nGPR[CMIPS::K0].nD0 = static_cast<int32>(KERNEL_STACK);
	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_INT;

	AssembleEventChecker();
	AssembleInterruptHandler();

	//Setup B0 table
	{
		uint32* table = reinterpret_cast<uint32*>(&m_ram[B0TABLE_BEGIN]);
		table[0x5B] = C0_EXCEPTIONHANDLER_BEGIN;
	}

	//Setup C0 table
	{
		uint32* table = reinterpret_cast<uint32*>(&m_ram[C0TABLE_BEGIN]);
		table[0x06] = C0_EXCEPTIONHANDLER_BEGIN;
	}

	//Assemble dummy exception handler
	{
		//0x70 = LUI
		//0x74 = ADDIU
		//Chrono Cross will overwrite the stuff present at the address that would be computed
		//by these two instructions and use something else
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + C0_EXCEPTIONHANDLER_BEGIN + 0x70));
		assembler.LI(CMIPS::T0, C0_EXCEPTIONHANDLER_BEGIN);
	}

	memset(m_events.GetBase(), 0, EVENTS_SIZE);
	LongJmpBuffer() = 0;
}

void CPsxBios::AssembleEventChecker()
{
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + EVENT_CHECKER));
	CMIPSAssembler::LABEL checkEventLabel = assembler.CreateLabel();
	CMIPSAssembler::LABEL doneEventLabel = assembler.CreateLabel();

	unsigned int currentEvent = CMIPS::S0;
	unsigned int eventMax = CMIPS::S1;
	unsigned int eventToCheck = CMIPS::S2;
	unsigned int needClearInt = CMIPS::S3;
	int stackAlloc = 5 * 4;

	//prolog
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
	assembler.SW(CMIPS::RA, 0x00, CMIPS::SP);
	assembler.SW(CMIPS::S0, 0x04, CMIPS::SP);
	assembler.SW(CMIPS::S1, 0x08, CMIPS::SP);
	assembler.SW(CMIPS::S2, 0x0C, CMIPS::SP);
	assembler.SW(CMIPS::S3, 0x10, CMIPS::SP);

	assembler.LI(currentEvent, EVENTS_BEGIN);
	assembler.LI(eventMax, EVENTS_BEGIN + EVENTS_SIZE);
	assembler.MOV(eventToCheck, CMIPS::A0);
	assembler.ADDU(needClearInt, CMIPS::R0, CMIPS::R0);

	//checkEvent
	{
		assembler.MarkLabel(checkEventLabel);
		
		//check if valid
		assembler.LW(CMIPS::T0, offsetof(EVENT, isValid), currentEvent);
		assembler.BEQ(CMIPS::T0, CMIPS::R0, doneEventLabel);
		assembler.NOP();
		
		//check if good event class
		assembler.LW(CMIPS::T0, offsetof(EVENT, classId), currentEvent);
		assembler.BNE(CMIPS::T0, eventToCheck, doneEventLabel);
		assembler.NOP();

		//Tell that we need to clear interrupt later (experimental)
		assembler.ADDIU(needClearInt, CMIPS::R0, 1);

		//check if enabled
		assembler.LW(CMIPS::T0, offsetof(EVENT, enabled), currentEvent);
		assembler.BEQ(CMIPS::T0, CMIPS::R0, doneEventLabel);
		assembler.NOP();

		//Start handler if present
		assembler.LW(CMIPS::T0, offsetof(EVENT, func), currentEvent);
		assembler.BEQ(CMIPS::T0, CMIPS::R0, doneEventLabel);
		assembler.NOP();

		assembler.JALR(CMIPS::T0);
		assembler.NOP();
	}

	//doneEvent
	assembler.MarkLabel(doneEventLabel);
	assembler.ADDIU(currentEvent, currentEvent, sizeof(EVENT));
	assembler.BNE(currentEvent, eventMax, checkEventLabel);
	assembler.NOP();

	//Result
	assembler.ADDU(CMIPS::V0, needClearInt, CMIPS::R0);

	//epilog
	assembler.LW(CMIPS::RA, 0x00, CMIPS::SP);
	assembler.LW(CMIPS::S0, 0x04, CMIPS::SP);
	assembler.LW(CMIPS::S1, 0x08, CMIPS::SP);
	assembler.LW(CMIPS::S2, 0x0C, CMIPS::SP);
	assembler.LW(CMIPS::S3, 0x10, CMIPS::SP);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);

	assembler.JR(CMIPS::RA);
	assembler.NOP();
}

void CPsxBios::AssembleInterruptHandler()
{
	//Assemble interrupt handler
	CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + INTR_HANDLER));
	CMIPSAssembler::LABEL returnExceptionLabel = assembler.CreateLabel();
	CMIPSAssembler::LABEL skipRootCounter2EventLabel = assembler.CreateLabel();

	//Get cause
	unsigned int cause = CMIPS::S3;
	assembler.LI(CMIPS::T0, CIntc::STATUS);
	assembler.LW(CMIPS::T0, 0, CMIPS::T0);
	assembler.LI(CMIPS::T1, CIntc::MASK);
	assembler.LW(CMIPS::T1, 0, CMIPS::T1);
	assembler.AND(cause, CMIPS::T0, CMIPS::T1);

	//Check if cause is root counter 2
	assembler.ANDI(CMIPS::T0, cause, 0x40);
	assembler.BEQ(CMIPS::T0, CMIPS::R0, skipRootCounter2EventLabel);
	assembler.NOP();

	assembler.LI(CMIPS::A0, EVENT_ID_RCNT2);
	assembler.JAL(EVENT_CHECKER);
	assembler.NOP();

	assembler.BEQ(CMIPS::V0, CMIPS::R0, skipRootCounter2EventLabel);
	assembler.NOP();

	//Clear cause
	assembler.LI(CMIPS::T0, CIntc::STATUS);
//	assembler.NOR(CMIPS::T1, CMIPS::R0, cause);
	assembler.LI(CMIPS::T1, ~0x40);
	assembler.SW(CMIPS::T1, 0, CMIPS::T0);

	assembler.MarkLabel(skipRootCounter2EventLabel);

	//checkIntHook
	assembler.LI(CMIPS::T0, LONGJMP_BUFFER);
	assembler.LW(CMIPS::T0, 0, CMIPS::T0);
	assembler.BEQ(CMIPS::T0, CMIPS::R0, returnExceptionLabel);
	assembler.NOP();

	//callIntHook
	assembler.ADDIU(CMIPS::A0, CMIPS::T0, CMIPS::R0);
	assembler.ADDIU(CMIPS::A1, CMIPS::R0, CMIPS::R0);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, 0xA0);
	assembler.ADDIU(CMIPS::T1, CMIPS::R0, 0x14);
	assembler.JR(CMIPS::T0);
	assembler.NOP();

	//ReturnFromException
	assembler.MarkLabel(returnExceptionLabel);
	assembler.ADDIU(CMIPS::T0, CMIPS::R0, 0xB0);
	assembler.ADDIU(CMIPS::T1, CMIPS::R0, 0x17);
	assembler.JR(CMIPS::T0);
	assembler.NOP();
}

void CPsxBios::LongJump(uint32 bufferAddress, uint32 value)
{
	bufferAddress = m_cpu.m_pAddrTranslator(&m_cpu, 0, bufferAddress);
	m_cpu.m_State.nGPR[CMIPS::RA].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x00));
	m_cpu.m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x04));
	m_cpu.m_State.nGPR[CMIPS::FP].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x08));
	m_cpu.m_State.nGPR[CMIPS::S0].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x0C));
	m_cpu.m_State.nGPR[CMIPS::S1].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x10));
	m_cpu.m_State.nGPR[CMIPS::S2].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x14));
	m_cpu.m_State.nGPR[CMIPS::S3].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x18));
	m_cpu.m_State.nGPR[CMIPS::S4].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x1C));
	m_cpu.m_State.nGPR[CMIPS::S5].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x20));
	m_cpu.m_State.nGPR[CMIPS::S6].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x24));
	m_cpu.m_State.nGPR[CMIPS::S7].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x28));
	m_cpu.m_State.nGPR[CMIPS::GP].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x2C));
	m_cpu.m_State.nGPR[CMIPS::V0].nD0 = value == 0 ? 1 : value;
}

uint32& CPsxBios::LongJmpBuffer() const
{
	return *reinterpret_cast<uint32*>(&m_ram[LONGJMP_BUFFER]);
}

void CPsxBios::SaveCpuState()
{
	uint32 address = m_cpu.m_State.nGPR[CMIPS::K0].nV0;
	for(unsigned int i = 0; i < 32; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_cpu.m_pMemoryMap->SetWord(address, m_cpu.m_State.nGPR[i].nV0);
		address += 4;
	}
}

void CPsxBios::LoadCpuState()
{
	uint32 address = m_cpu.m_State.nGPR[CMIPS::K0].nV0;
	for(unsigned int i = 0; i < 32; i++)
	{
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
		m_cpu.m_State.nGPR[i].nD0 = static_cast<int32>(m_cpu.m_pMemoryMap->GetWord(address));
		address += 4;
	}
}

void CPsxBios::HandleInterrupt()
{
	if(m_cpu.GenerateInterrupt(0xBFC00000))
	{
		SaveCpuState();
		uint32 status = m_cpu.m_pMemoryMap->GetWord(CIntc::STATUS);
		uint32 mask = m_cpu.m_pMemoryMap->GetWord(CIntc::MASK);
		uint32 cause = status & mask;
		for(unsigned int i = 1; i <= MAX_EVENT; i++)
		{
			EVENT* eventPtr = m_events[i];
			if(eventPtr == NULL) continue;
			if(cause & 0x08 && eventPtr->classId == 0xF0000009)
			{
				eventPtr->fired = 1;		
			}
		}
		m_cpu.m_State.nPC = INTR_HANDLER;
//		m_cpu.m_pMemoryMap->SetWord(CIntc::STATUS, ~0x40);
	}
}

void CPsxBios::HandleException()
{
	assert(m_cpu.m_State.nHasException);
//    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
	uint32 searchAddress = m_cpu.m_State.nGPR[CMIPS::K1].nV0;
    uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
    if(callInstruction != 0x0000000C)
    {
        throw runtime_error("Not a SYSCALL.");
    }
#ifdef _DEBUG
	DisassembleSyscall(searchAddress);
#endif
	if(searchAddress == 0xA0)
	{
		ProcessSubFunction(m_handlerA0, MAX_HANDLER_A0);
	}
	else if(searchAddress == 0xB0)
	{
		ProcessSubFunction(m_handlerB0, MAX_HANDLER_B0);
	}
	else if(searchAddress == 0xC0)
	{
		ProcessSubFunction(m_handlerC0, MAX_HANDLER_C0);
	}
	else
	{
		uint32 functionId = m_cpu.m_State.nGPR[CMIPS::A0].nV0;
		switch(functionId)
		{
		case 0x01:
			sc_EnterCriticalSection();
			break;
		case 0x02:
			sc_ExitCriticalSection();
			break;
		default:
			sc_Illegal();
			break;
		}
	}
	m_cpu.m_State.nHasException = 0;
}

void CPsxBios::ProcessSubFunction(SyscallHandler* handlerTable, unsigned int handlerTableLength)
{
	uint32 functionId = m_cpu.m_State.nGPR[CMIPS::T1].nV0;
	if(functionId >= handlerTableLength)
	{
		sc_Illegal();
	}
	functionId %= handlerTableLength;
	((this)->*(handlerTable[functionId]))();
}

void CPsxBios::DisassembleSyscall(uint32 searchAddress)
{
	if(searchAddress == 0x00A0)
	{
		uint32 functionId = m_cpu.m_State.nGPR[CMIPS::T1].nV0;
		switch(functionId)
		{
		case 0x13:
			CLog::GetInstance().Print(LOG_NAME, "setjmp(buffer = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x14:
			CLog::GetInstance().Print(LOG_NAME, "longjmp(buffer = 0x%0.8X, value = %i);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x19:
			CLog::GetInstance().Print(LOG_NAME, "strcpy(dst = 0x%0.8X, src = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x28:
			CLog::GetInstance().Print(LOG_NAME, "bzero(address = 0x%0.8X, length = 0x%x);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x2B:
			CLog::GetInstance().Print(LOG_NAME, "memset(address = 0x%0.8X, value = 0x%x, length = 0x%x);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0,
				m_cpu.m_State.nGPR[SC_PARAM2].nV0);
			break;
		case 0x39:
			CLog::GetInstance().Print(LOG_NAME, "InitHeap(block = 0x%0.8X, n = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x3F:
			CLog::GetInstance().Print(LOG_NAME, "printf(fmt = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x44:
			CLog::GetInstance().Print(LOG_NAME, "FlushCache();\r\n");
			break;
		case 0x70:
			CLog::GetInstance().Print(LOG_NAME, "_bu_init();\r\n");
			break;
		case 0x72:
			CLog::GetInstance().Print(LOG_NAME, "_96_remove();\r\n");
			break;
		case 0x9F:
			CLog::GetInstance().Print(LOG_NAME, "SetMem(size = %i);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Unknown system call encountered (0xA0, 0x%X).\r\n", functionId);
			break;
		}
	}
	else if(searchAddress == 0x00B0)
	{
		uint32 functionId = m_cpu.m_State.nGPR[CMIPS::T1].nV0;
		switch(functionId)
		{
		case 0x00:
			CLog::GetInstance().Print(LOG_NAME, "SysMalloc(size = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x07:
			CLog::GetInstance().Print(LOG_NAME, "DeliverEvent(class = 0x%X, event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x08:
			CLog::GetInstance().Print(LOG_NAME, "OpenEvent(class = 0x%X, spec = 0x%X, mode = 0x%X, func = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0,
				m_cpu.m_State.nGPR[SC_PARAM2].nV0,
				m_cpu.m_State.nGPR[SC_PARAM3].nV0);
			break;
		case 0x0A:
			CLog::GetInstance().Print(LOG_NAME, "WaitEvent(event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x0B:
			CLog::GetInstance().Print(LOG_NAME, "TestEvent(event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x0C:
			CLog::GetInstance().Print(LOG_NAME, "EnableEvent(event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x0D:
			CLog::GetInstance().Print(LOG_NAME, "DisableEvent(event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x16:
			CLog::GetInstance().Print(LOG_NAME, "PAD_dr();\r\n");
			break;
		case 0x17:
			CLog::GetInstance().Print(LOG_NAME, "ReturnFromException();\r\n");
			break;
		case 0x19:
			CLog::GetInstance().Print(LOG_NAME, "HookEntryInt(address = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x4A:
			CLog::GetInstance().Print(LOG_NAME, "InitCARD();\r\n");
			break;
		case 0x4B:
			CLog::GetInstance().Print(LOG_NAME, "StartCARD();\r\n");
			break;
		case 0x56:
			CLog::GetInstance().Print(LOG_NAME, "GetC0Table();\r\n");
			break;
		case 0x57:
			CLog::GetInstance().Print(LOG_NAME, "GetB0Table();\r\n");
			break;
		case 0x5B:
			CLog::GetInstance().Print(LOG_NAME, "ChangeClearPad(param = %i);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Unknown system call encountered (0xB0, 0x%X).\r\n", functionId);
			break;
		}
	}
	else if(searchAddress == 0x00C0)
	{
		uint32 functionId = m_cpu.m_State.nGPR[CMIPS::T1].nV0;
		switch(functionId)
		{
		case 0x03:
			CLog::GetInstance().Print(LOG_NAME, "SysDeqIntRP(index = %i, queue = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		case 0x08:
			CLog::GetInstance().Print(LOG_NAME, "SysInitMemory();\r\n");
			break;
		case 0x0A:
			CLog::GetInstance().Print(LOG_NAME, "ChangeClearRCnt(param0 = %i, param1 = %i);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Unknown system call encountered (0xC0, 0x%X).\r\n", functionId);
			break;
		}
	}
	else
	{
		uint32 functionId = m_cpu.m_State.nGPR[CMIPS::A0].nV0;
		switch(functionId)
		{
		case 0x01:
			CLog::GetInstance().Print(LOG_NAME, "EnterCriticalSection();\r\n");
			break;
		case 0x02:
			CLog::GetInstance().Print(LOG_NAME, "ExitCriticalSection();\r\n");
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Unknown system call encountered.\r\n");
			break;
		}
	}
}

//A0 - 13
void CPsxBios::sc_setjmp()
{
	uint32 bufferAddress = m_cpu.m_pAddrTranslator(&m_cpu, 0, m_cpu.m_State.nGPR[SC_PARAM0].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x00, m_cpu.m_State.nGPR[CMIPS::RA].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x04, m_cpu.m_State.nGPR[CMIPS::SP].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x08, m_cpu.m_State.nGPR[CMIPS::FP].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x0C, m_cpu.m_State.nGPR[CMIPS::S0].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x10, m_cpu.m_State.nGPR[CMIPS::S1].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x14, m_cpu.m_State.nGPR[CMIPS::S2].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x18, m_cpu.m_State.nGPR[CMIPS::S3].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x1C, m_cpu.m_State.nGPR[CMIPS::S4].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x20, m_cpu.m_State.nGPR[CMIPS::S5].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x24, m_cpu.m_State.nGPR[CMIPS::S6].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x28, m_cpu.m_State.nGPR[CMIPS::S7].nV0);
	m_cpu.m_pMemoryMap->SetWord(bufferAddress + 0x2C, m_cpu.m_State.nGPR[CMIPS::GP].nV0);
	m_cpu.m_State.nGPR[CMIPS::V0].nD0 = 0;
}

//A0 - 14
void CPsxBios::sc_longjmp()
{
	uint32 buffer = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 value = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
	LongJump(buffer, value);
}

//A0 - 19
void CPsxBios::sc_strcpy()
{
	uint32 dst = m_cpu.m_pAddrTranslator(&m_cpu, 0, m_cpu.m_State.nGPR[SC_PARAM0].nV0);
	uint32 src = m_cpu.m_pAddrTranslator(&m_cpu, 0, m_cpu.m_State.nGPR[SC_PARAM1].nV0);

	strcpy(
		reinterpret_cast<char*>(m_ram + dst),
		reinterpret_cast<char*>(m_ram + src)
		);

	m_cpu.m_State.nGPR[SC_RETURN].nV0 = dst;
}

//A0 - 28
void CPsxBios::sc_bzero()
{
	uint32 address = m_cpu.m_pAddrTranslator(&m_cpu, 0, m_cpu.m_State.nGPR[SC_PARAM0].nV0);
	uint32 length = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
	if((address + length) > CPsxVm::RAMSIZE)
	{
		throw exception();
	}
	memset(m_ram + address, 0, length);
}

//A0 - 2B
void CPsxBios::sc_memset()
{
	uint32 address = m_cpu.m_pAddrTranslator(&m_cpu, 0, m_cpu.m_State.nGPR[SC_PARAM0].nV0);
	uint8 value = static_cast<uint8>(m_cpu.m_State.nGPR[SC_PARAM1].nV0);
	uint32 length = m_cpu.m_State.nGPR[SC_PARAM2].nV0;
	if((address + length) > CPsxVm::RAMSIZE)
	{
		throw exception();
	}
	memset(m_ram + address, value, length);

	m_cpu.m_State.nGPR[SC_RETURN].nV0 = address;
}

//A0 - 39
void CPsxBios::sc_InitHeap()
{
	uint32 block = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 n = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
}

//A0 - 3F
void CPsxBios::sc_printf()
{
	uint32 formatAddress = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
}

//A0 - 44
void CPsxBios::sc_FlushCache()
{

}

//A0 - 70
void CPsxBios::sc_bu_init()
{
	
}

//A0 - 72
void CPsxBios::sc_96_remove()
{
	
}

//A0 - 9F
void CPsxBios::sc_SetMem()
{
	uint32 n = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
}

//B0 - 00
void CPsxBios::sc_SysMalloc()
{
	uint32 length = m_cpu.m_State.nGPR[SC_PARAM0].nV0;

	m_cpu.m_State.nGPR[SC_RETURN].nV0 = 0;
}

//B0 - 07
void CPsxBios::sc_DeliverEvent()
{
	uint32 classId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
}

//B0 - 08
void CPsxBios::sc_OpenEvent()
{
	uint32 classId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 spec = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
	uint32 mode = m_cpu.m_State.nGPR[SC_PARAM2].nV0;
	uint32 func = m_cpu.m_State.nGPR[SC_PARAM3].nV0;

	uint32 eventId = m_events.Allocate();
	if(eventId == -1)
	{
		throw exception();
	}
	EVENT* eventPtr = m_events[eventId];
	eventPtr->classId	= classId;
	eventPtr->spec		= spec;
	eventPtr->mode		= mode;
	eventPtr->func		= func;
	eventPtr->fired		= 0;

	m_cpu.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(eventId);
}

//B0 - 0A
void CPsxBios::sc_WaitEvent()
{
	uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
}

//B0 - 0B
void CPsxBios::sc_TestEvent()
{
	uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	EVENT* eventPtr = m_events[eventId];
	if(eventPtr != NULL)
	{
		m_cpu.m_State.nGPR[SC_RETURN].nD0 = eventPtr->fired;
//		m_cpu.m_State.nGPR[SC_RETURN].nD0 = 0;
	}
}

//B0 - 0C
void CPsxBios::sc_EnableEvent()
{
	try
	{
		uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
		EVENT* eventPtr = m_events[eventId];
		if(eventPtr != NULL)
		{
			eventPtr->enabled = true;
			eventPtr->fired = 0;
		}
	}
	catch(...)
	{
		m_cpu.m_State.nGPR[SC_RETURN].nD0 = -1;
	}
}

//B0 - 0D
void CPsxBios::sc_DisableEvent()
{
	try
	{
		uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
		EVENT* eventPtr = m_events[eventId];
		if(eventPtr != NULL)
		{
			eventPtr->enabled = false;
		}
	}
	catch(...)
	{
		m_cpu.m_State.nGPR[SC_RETURN].nD0 = -1;
	}
}

//B0 - 16
void CPsxBios::sc_PAD_dr()
{
	
}

//B0 - 17
void CPsxBios::sc_ReturnFromException()
{
	uint32& status = m_cpu.m_State.nCOP0[CCOP_SCU::STATUS];
	if(status & CMIPS::STATUS_ERL)
	{
		m_cpu.m_State.nPC = m_cpu.m_State.nCOP0[CCOP_SCU::ERROREPC];
		status &= ~CMIPS::STATUS_ERL;
	}
	else if(status & CMIPS::STATUS_EXL)
	{
		m_cpu.m_State.nPC = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
		status &= ~CMIPS::STATUS_EXL;
	}
	LoadCpuState();
}

//B0 - 19
void CPsxBios::sc_HookEntryInt()
{
	uint32 address = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	LongJmpBuffer() = address;
}

//B0 - 4A
void CPsxBios::sc_InitCARD()
{
	
}

//B0 - 4B
void CPsxBios::sc_StartCARD()
{
	
}

//B0 - 56
void CPsxBios::sc_GetC0Table()
{
	m_cpu.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(C0TABLE_BEGIN);
}

//B0 - 57
void CPsxBios::sc_GetB0Table()
{
	m_cpu.m_State.nGPR[SC_RETURN].nD0 = static_cast<int32>(B0TABLE_BEGIN);
}

//B0 - 5B
void CPsxBios::sc_ChangeClearPad()
{
	uint32 param = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
}

//C0 - 03
void CPsxBios::sc_SysDeqIntRP()
{
	uint32 index = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 queue = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
}

//C0 - 08
void CPsxBios::sc_SysInitMemory()
{

}

//C0 - 0A
void CPsxBios::sc_ChangeClearRCnt()
{
	uint32 param0 = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 param1 = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
}

void CPsxBios::sc_EnterCriticalSection()
{

}

void CPsxBios::sc_ExitCriticalSection()
{

}

void CPsxBios::sc_Illegal()
{
#ifdef _DEBUG
	throw runtime_error("Illegal system call.");
#endif
}

CPsxBios::SyscallHandler CPsxBios::m_handlerA0[MAX_HANDLER_A0] = 
{
	//0x00
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x08
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x10
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_setjmp,			&sc_longjmp,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x18
	&sc_Illegal,		&sc_strcpy,			&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x20
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x28
	&sc_bzero,			&sc_Illegal,		&sc_Illegal,		&sc_memset,			&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x30
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x38
	&sc_Illegal,		&sc_InitHeap,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_printf,
	//0x40
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_FlushCache,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x48
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x50
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x58
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x60
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x68
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x70
	&sc_bu_init,		&sc_Illegal,		&sc_96_remove,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x78
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x80
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x88
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x90
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x98
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_SetMem,
	//0xA0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xA8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xB0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xB8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xC0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xC8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xD0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xD8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xE0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xE8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xF0
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0xF8
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal
};

CPsxBios::SyscallHandler CPsxBios::m_handlerB0[MAX_HANDLER_B0] = 
{
	//0x00
	&sc_SysMalloc,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_DeliverEvent,
	//0x08
	&sc_OpenEvent,		&sc_Illegal,		&sc_WaitEvent,		&sc_TestEvent,		&sc_EnableEvent,	&sc_DisableEvent,	&sc_Illegal,		&sc_Illegal,
	//0x10
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_PAD_dr,			&sc_ReturnFromException,
	//0x18
	&sc_Illegal,		&sc_HookEntryInt,	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x20
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x28
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x30
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x38
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x40
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x48
	&sc_Illegal,		&sc_Illegal,		&sc_InitCARD,		&sc_StartCARD,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x50
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_GetC0Table,		&sc_GetB0Table,
	//0x58
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_ChangeClearPad,	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x60
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x68
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x70
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x78
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal
};

CPsxBios::SyscallHandler CPsxBios::m_handlerC0[MAX_HANDLER_C0] = 
{
	//0x00
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_SysDeqIntRP,	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x08
	&sc_SysInitMemory,	&sc_Illegal,		&sc_ChangeClearRCnt,&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x10
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x18
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal
};
