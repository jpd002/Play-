#include <stdio.h>
#include <string.h>
#include "PtrMacro.h"
#include "MIPS.h"
#include "COP_SCU.h"

const char* CMIPS::m_sGPRName[] =
{
	"R0",	"AT",	"V0",	"V1",	"A0",	"A1",	"A2",	"A3",
	"T0",	"T1",	"T2",	"T3",	"T4",	"T5",	"T6",	"T7",
	"S0",	"S1",	"S2",	"S3",	"S4",	"S5",	"S6",	"S7",
	"T8",	"T9",	"K0",	"K1",	"GP",	"SP",	"FP",	"RA"
};

CMIPS::CMIPS(MEMORYMAP_ENDIANESS nEnd, uint32 nExecStart, uint32 nExecEnd)
{
	m_pAnalysis = new CMIPSAnalysis(this);
	switch(nEnd)
	{
	case MEMORYMAP_ENDIAN_LSBF:
		m_pMemoryMap = new CMemoryMap_LSBF;
		break;
	case MEMORYMAP_ENDIAN_MSBF:
		//
		break;
	}

	m_pCOP[0] = NULL;
	m_pCOP[1] = NULL;
	m_pCOP[2] = NULL;
	m_pCOP[3] = NULL;

	m_nIllOpcode = MIPS_INVALID_PC;
	m_pSysCallHandler = DefaultSysCallHandler;
}

CMIPS::~CMIPS()
{
	DELETEPTR(m_pMemoryMap);
	DELETEPTR(m_pAnalysis);
}

void CMIPS::ToggleBreakpoint(uint32 address)
{
    if(m_breakpoints.find(address) != m_breakpoints.end())
	{
        m_breakpoints.erase(address);
		return;
	}
    m_breakpoints.insert(address);
}

//void CMIPS::Step()
//{
//	Execute(1);
//}

//RET_CODE CMIPS::Execute(int nQuota)
//{
//	uint32 nPC;
//	CCacheBlock* pB;
//
//	m_nQuota = nQuota;
//
//	nPC = m_State.nPC;
//	nPC = m_pAddrTranslator(this, 0x00000000, nPC);
//
//	pB = m_pExecMap->FindBlock(nPC);
//	if(pB == NULL)
//	{
//		pB = m_pExecMap->CreateBlock(nPC);
//	}
//	return pB->Execute(this);
//}

long CMIPS::GetBranch(uint16 nData)
{
	if(nData & 0x8000)
	{
		return -((0x10000 - nData) * 4);
	}
	else
	{
		return ((nData & 0x7FFF) * 4);
	}
}

bool CMIPS::IsBranch(uint32 nAddress)
{
	uint32 nOpcode;
	nOpcode = m_pMemoryMap->GetWord(nAddress);
	return m_pArch->IsInstructionBranch(this, nAddress, nOpcode);
}

uint32 CMIPS::TranslateAddress64(CMIPS* pC, uint32 nVAddrHI, uint32 nVAddrLO)
{
	//Proper address translation?
	return nVAddrLO & 0x1FFFFFFF;
}

void CMIPS::DefaultSysCallHandler(CMIPS* context)
{
	printf("MIPS: Unhandled SYSCALL encountered.\r\n");
}

bool CMIPS::GenerateInterrupt(uint32 nAddress)
{
	//Check if interrupts are enabled
	if(!(m_State.nCOP0[CCOP_SCU::STATUS] & 0x0001)) return false;

	//Check if we're in exception mode (interrupts are disabled in exception mode)
	if(m_State.nCOP0[CCOP_SCU::STATUS] & 0x0002) return false;

	return CMIPS::GenerateException(nAddress);
}

bool CMIPS::GenerateException(uint32 nAddress)
{
	//Save exception PC
	if(m_State.nDelayedJumpAddr != MIPS_INVALID_PC)
	{
		m_State.nCOP0[CCOP_SCU::EPC] = m_State.nPC - 4;
		//m_State.nCOP0[CCOP_SCU::EPC] = m_State.nDelayedJumpAddr;
	}
	else
	{
		m_State.nCOP0[CCOP_SCU::EPC] = m_State.nPC;
	}

	m_State.nDelayedJumpAddr = MIPS_INVALID_PC;

	m_State.nPC = nAddress;

	//Set in exception mode
	m_State.nCOP0[CCOP_SCU::STATUS] |= 0x02;

	//Perhaps, we should change this...
	m_nQuota = 1;

	return true;
}

//void CMIPS::InvalidateCache()
//{
//	m_pExecMap->InvalidateBlocks();
//}

bool CMIPS::MustBreak()
{
	return m_breakpoints.find(m_State.nPC) != m_breakpoints.end();
}
