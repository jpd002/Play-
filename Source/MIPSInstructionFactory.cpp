#include <assert.h>
#include <stddef.h>
#include "MIPSInstructionFactory.h"
#include "CodeGen.h"
#include "MIPS.h"
#include "PtrMacro.h"
#include "offsetof_def.h"

using namespace std;

CMIPS*				CMIPSInstructionFactory::m_pCtx		= NULL;
CCodeGen*           CMIPSInstructionFactory::m_codeGen  = NULL;
uint32				CMIPSInstructionFactory::m_nAddress = 0;
uint32				CMIPSInstructionFactory::m_nOpcode	= 0;

CMIPSInstructionFactory::CMIPSInstructionFactory(MIPS_REGSIZE nRegSize)
{
	assert(nRegSize == MIPS_REGSIZE_64);
}

CMIPSInstructionFactory::~CMIPSInstructionFactory()
{

}

void CMIPSInstructionFactory::SetupQuickVariables(uint32 nAddress, CCodeGen* codeGen, CMIPS* pCtx)
{
	m_pCtx			= pCtx;
    m_codeGen       = codeGen;
	m_nAddress		= nAddress;

	m_nOpcode		= m_pCtx->m_pMemoryMap->GetInstruction(m_nAddress);
}

void CMIPSInstructionFactory::ComputeMemAccessAddr()
{
	uint8 nRS;
	uint16 nImmediate;

	nRS			= (uint8) ((m_nOpcode >> 21) & 0x001F);
	nImmediate	= (uint16)((m_nOpcode >>  0) & 0xFFFF);

	//TODO: Compute the complete 64-bit address

	//Translate the address

	//Push context
	m_codeGen->PushRef(m_pCtx);

	//Push high part
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[1]));

	//Push low part of address
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[0]));
	if(nImmediate != 0)
	{
		m_codeGen->PushCst((int16)nImmediate);
		m_codeGen->Add();
	}

	//Call
	m_codeGen->Call(reinterpret_cast<void*>(m_pCtx->m_pAddrTranslator), 3, true);
}

void CMIPSInstructionFactory::Branch(bool nCondition)
{
	uint16 nImmediate = (uint16)(m_nOpcode & 0xFFFF);

	m_codeGen->PushCst(MIPS_INVALID_PC);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	m_codeGen->BeginIf(nCondition);
	{
        m_codeGen->PushCst((m_nAddress + 4) + CMIPS::GetBranch(nImmediate));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::BranchLikely(bool conditionValue)
{
	uint16 nImmediate = (uint16)(m_nOpcode & 0xFFFF);

	m_codeGen->PushCst(MIPS_INVALID_PC);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	m_codeGen->BeginIfElse(conditionValue);
	{
        m_codeGen->PushCst((m_nAddress + 4) + CMIPS::GetBranch(nImmediate));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->BeginIfElseAlt();
	{
        m_codeGen->PushCst(m_nAddress + 8);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->EndQuota();
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::Illegal()
{
    throw runtime_error("Illegal instruction.");
}
