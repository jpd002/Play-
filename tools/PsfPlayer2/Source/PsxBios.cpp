#include "PsxBios.h"
#include "COP_SCU.h"
#include "Log.h"

#define LOG_NAME	("psxbios")
#define SC_PARAM0	(CMIPS::A0)
#define SC_PARAM1	(CMIPS::A1)
#define SC_PARAM2	(CMIPS::A2)
#define SC_PARAM3	(CMIPS::A3)
#define SC_RETURN	(CMIPS::V0)

using namespace std;

CPsxBios::CPsxBios(CMIPS& cpu) :
m_cpu(cpu)
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
		//SYSCALL and JR RA
		m_cpu.m_pMemoryMap->SetWord(syscallAddress[i] + 0x00, 0x0000000C);
		m_cpu.m_pMemoryMap->SetWord(syscallAddress[i] + 0x04, 0x03E00008);
	}

	m_longJmpBuffer = 0;
}

void CPsxBios::LongJump(uint32 bufferAddress)
{
	m_cpu.m_State.nGPR[CMIPS::RA].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x00);
	m_cpu.m_State.nGPR[CMIPS::SP].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x04);
	m_cpu.m_State.nGPR[CMIPS::FP].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x08);
	m_cpu.m_State.nGPR[CMIPS::S0].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x0C);
	m_cpu.m_State.nGPR[CMIPS::S1].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x10);
	m_cpu.m_State.nGPR[CMIPS::S2].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x14);
	m_cpu.m_State.nGPR[CMIPS::S3].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x18);
	m_cpu.m_State.nGPR[CMIPS::S4].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x1C);
	m_cpu.m_State.nGPR[CMIPS::S5].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x20);
	m_cpu.m_State.nGPR[CMIPS::S6].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x24);
	m_cpu.m_State.nGPR[CMIPS::S7].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x28);
	m_cpu.m_State.nGPR[CMIPS::GP].nV0 = m_cpu.m_pMemoryMap->GetWord(bufferAddress + 0x2C);
}

void CPsxBios::HandleException()
{
	assert(m_cpu.m_State.nHasException);
    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
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
		case 0x39:
			CLog::GetInstance().Print(LOG_NAME, "InitHeap(block = 0x%0.8X, n = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0);
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
		case 0x08:
			CLog::GetInstance().Print(LOG_NAME, "OpenEvent(class = 0x%X, spec = 0x%X, mode = 0x%X, func = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0,
				m_cpu.m_State.nGPR[SC_PARAM1].nV0,
				m_cpu.m_State.nGPR[SC_PARAM2].nV0,
				m_cpu.m_State.nGPR[SC_PARAM3].nV0);
			break;
		case 0x0C:
			CLog::GetInstance().Print(LOG_NAME, "EnableEvent(event = 0x%X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
			break;
		case 0x19:
			CLog::GetInstance().Print(LOG_NAME, "HookEntryInt(address = 0x%0.8X);\r\n",
				m_cpu.m_State.nGPR[SC_PARAM0].nV0);
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

//A0 - 39
void CPsxBios::sc_InitHeap()
{
	uint32 block = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 n = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
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

//B0 - 08
void CPsxBios::sc_OpenEvent()
{
	uint32 classId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	uint32 spec = m_cpu.m_State.nGPR[SC_PARAM1].nV0;
	uint32 mode = m_cpu.m_State.nGPR[SC_PARAM2].nV0;
	uint32 func = m_cpu.m_State.nGPR[SC_PARAM3].nV0;

	m_cpu.m_State.nGPR[SC_RETURN].nV0 = 0xDEADBEEF;
}

//B0 - 0C
void CPsxBios::sc_EnableEvent()
{
	uint32 eventId = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
}

//B0 - 19
void CPsxBios::sc_HookEntryInt()
{
	uint32 address = m_cpu.m_State.nGPR[SC_PARAM0].nV0;
	m_longJmpBuffer = address;
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
	throw runtime_error("Illegal system call.");
}

CPsxBios::SyscallHandler CPsxBios::m_handlerA0[MAX_HANDLER_A0] = 
{
	//0x00
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x08
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x10
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x18
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x20
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x28
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x30
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x38
	&sc_Illegal,		&sc_InitHeap,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x40
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
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
	&sc_SysMalloc,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x08
	&sc_OpenEvent,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_EnableEvent,	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x10
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
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
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
	//0x50
	&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,		&sc_Illegal,
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
