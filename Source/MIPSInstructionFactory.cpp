#include <assert.h>
#include <stddef.h>
#include "MIPSInstructionFactory.h"
#include "MipsCodeGen.h"
#include "CacheBlock.h"
#include "MIPS.h"
#include "PtrMacro.h"
#include "offsetof_def.h"

CMIPS*				CMIPSInstructionFactory::m_pCtx		= NULL;
CCacheBlock*		CMIPSInstructionFactory::m_pB		= NULL;
CMipsCodeGen*       CMIPSInstructionFactory::m_codeGen  = NULL;
uint32				CMIPSInstructionFactory::m_nAddress = 0;
uint32				CMIPSInstructionFactory::m_nOpcode	= 0;

CMIPSInstructionFactory::CMIPSInstructionFactory(MIPS_REGSIZE nRegSize)
{
	assert(nRegSize == MIPS_REGSIZE_64);
}

CMIPSInstructionFactory::~CMIPSInstructionFactory()
{

}

void CMIPSInstructionFactory::SetupQuickVariables(uint32 nAddress, CCacheBlock* pBlock, CMIPS* pCtx)
{
	m_pCtx			= pCtx;
//	m_pB			= pBlock;
    m_codeGen       = reinterpret_cast<CMipsCodeGen*>(pBlock);
	m_nAddress		= nAddress;

	m_nOpcode		= m_pCtx->m_pMemoryMap->GetWord(m_nAddress);
}

void CMIPSInstructionFactory::ComputeMemAccessAddr()
{
	uint8 nRS;
	uint16 nImmediate;

	nRS			= (uint8) ((m_nOpcode >> 21) & 0x1F);
	nImmediate	= (uint16)(m_nOpcode & 0xFFFF);

	//TODO: Compute the complete 64-bit address (carry)

	//Translate the address
	//Push low part of address
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[nRS].nV[0]);
	m_pB->AddImm((int16)nImmediate);
	//Push high part of address
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[nRS].nV[1]);
	//Push context address
	m_pB->PushRef(m_pCtx);
	m_pB->Call(reinterpret_cast<void*>(m_pCtx->m_pAddrTranslator), 3, true);	
}

void CMIPSInstructionFactory::SignExtendTop32(unsigned int nTarget)
{
	//64-bits only here
	m_pB->SeX32();
	
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[nTarget].nV[1]);
}

void CMIPSInstructionFactory::Branch(bool nCondition)
{
	uint16 nImmediate;

	nImmediate	= (uint16)(m_nOpcode & 0xFFFF);

	m_pB->PushImm(MIPS_INVALID_PC);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->BeginJcc(nCondition);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nPC);
		m_pB->AddImm(CMIPS::GetBranch(nImmediate));
		m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	m_pB->EndJcc();

	m_pB->SetDelayedJumpCheck();
}

void CMIPSInstructionFactory::BranchLikely(bool nCondition)
{
	uint16 nImmediate;

	nImmediate	= (uint16)(m_nOpcode & 0xFFFF);

	m_pB->PushImm(MIPS_INVALID_PC);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->PushTop();

	m_pB->BeginJcc(nCondition);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nPC);
		m_pB->AddImm(CMIPS::GetBranch(nImmediate));
		m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	m_pB->EndJcc();

	m_pB->BeginJcc(!nCondition);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nPC);
		m_pB->AddImm(4);
		m_pB->PullAddr(&m_pCtx->m_State.nPC);
	}
	m_pB->EndJcc();

	m_pB->SetProgramCounterChanged();
	m_pB->SetDelayedJumpCheck();
}

void CMIPSInstructionFactory::ComputeMemAccessAddrEx()
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

void CMIPSInstructionFactory::BranchEx(bool nCondition)
{
	uint16 nImmediate;

	nImmediate = (uint16)(m_nOpcode & 0xFFFF);

	m_codeGen->PushCst(MIPS_INVALID_PC);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	m_codeGen->BeginIf(nCondition);
	{
//		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
//		m_codeGen->PushCst(CMIPS::GetBranch(nImmediate));
//		m_codeGen->Add();
        m_codeGen->PushCst((m_nAddress + 4) + CMIPS::GetBranch(nImmediate));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->EndIf();

//	m_pB->SetDelayedJumpCheck();
}

void CMIPSInstructionFactory::Illegal()
{
#ifdef _DEBUG
	m_pB->PushAddr(&m_pCtx->m_State.nPC);
	m_pB->SubImm(4);
	m_pB->PullAddr(&m_pCtx->m_nIllOpcode);
#endif
}
