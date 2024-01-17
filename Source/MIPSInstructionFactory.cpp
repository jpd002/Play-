#include <assert.h>
#include <stddef.h>
#include "MIPSInstructionFactory.h"
#include "MIPS.h"
#include "offsetof_def.h"
#include "BitManip.h"
#include "COP_SCU.h"

extern "C" void TrapHandler(CMIPS* context)
{
	//This is just a way to inspect state when traps are happening
	assert(false && "Unhandled trap.");
}

CMIPSInstructionFactory::CMIPSInstructionFactory(MIPS_REGSIZE nRegSize)
    : m_regSize(nRegSize)
{
}

void CMIPSInstructionFactory::SetupQuickVariables(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx, uint32 instrPosition)
{
	m_pCtx = pCtx;
	m_codeGen = codeGen;
	m_nAddress = nAddress;
	m_instrPosition = instrPosition;

	m_nOpcode = m_pCtx->m_pMemoryMap->GetInstruction(m_nAddress);
}

static void HandleTLBException(CMIPS*)
{
	//Will exit CPU execution loop with an exception pending
}

void CMIPSInstructionFactory::CheckTLBExceptions(bool isWrite)
{
	if(m_pCtx->m_pAddrTranslator == &CMIPS::TranslateAddress64) return;
	if(m_pCtx->m_TLBExceptionChecker == nullptr) return;

	uint8 nRS = (uint8)((m_nOpcode >> 21) & 0x001F);
	uint16 nImmediate = (uint16)((m_nOpcode >> 0) & 0xFFFF);

	//Push context
	m_codeGen->PushCtx();

	//Push low part of address
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[0]));
	if(nImmediate != 0)
	{
		m_codeGen->PushCst((int16)nImmediate);
		m_codeGen->Add();
	}

	//Push isWrite
	m_codeGen->PushCst(isWrite ? 1 : 0);

	//Call
	m_codeGen->Call(reinterpret_cast<void*>(m_pCtx->m_TLBExceptionChecker), 3, Jitter::CJitter::RETURN_VALUE_32);

	m_codeGen->PushCst(MIPS_EXCEPTION_NONE);
	m_codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst(m_instrPosition);
		m_codeGen->Add();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[CCOP_SCU::EPC]));
		m_codeGen->JumpTo(reinterpret_cast<void*>(&HandleTLBException));
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::CheckTrap()
{
	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		m_codeGen->PushCtx();
		m_codeGen->Call(reinterpret_cast<void*>(&TrapHandler), 1, Jitter::CJitter::RETURN_VALUE_NONE);
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::ComputeMemAccessAddr()
{
	uint8 nRS = (uint8)((m_nOpcode >> 21) & 0x001F);
	uint16 nImmediate = (uint16)((m_nOpcode >> 0) & 0xFFFF);

	if(m_pCtx->m_pAddrTranslator == &CMIPS::TranslateAddress64)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[0]));
		if(nImmediate != 0)
		{
			m_codeGen->PushCst((int16)nImmediate);
			m_codeGen->Add();
		}
		m_codeGen->PushCst(0x1FFFFFFF);
		m_codeGen->And();
	}
	else
	{
		//TODO: Compute the complete 64-bit address

		//Translate the address

		//Push context
		m_codeGen->PushCtx();

		//Push low part of address
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[0]));
		if(nImmediate != 0)
		{
			m_codeGen->PushCst((int16)nImmediate);
			m_codeGen->Add();
		}

		//Call
		m_codeGen->Call(reinterpret_cast<void*>(m_pCtx->m_pAddrTranslator), 2, Jitter::CJitter::RETURN_VALUE_32);
	}
}

void CMIPSInstructionFactory::ComputeMemAccessAddrNoXlat()
{
	uint8 nRS = (uint8)((m_nOpcode >> 21) & 0x001F);
	uint16 nImmediate = (uint16)((m_nOpcode >> 0) & 0xFFFF);

	//Push low part of address
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[nRS].nV[0]));
	if(nImmediate != 0)
	{
		m_codeGen->PushCst((int16)nImmediate);
		m_codeGen->Add();
	}
}

void CMIPSInstructionFactory::ComputeMemAccessRef(uint32 accessSize)
{
	ComputeMemAccessRefIdx(accessSize);
	m_codeGen->AddRef();
}

void CMIPSInstructionFactory::ComputeMemAccessRefIdx(uint32 accessSize)
{
	ComputeMemAccessPageRef();

	auto rs = static_cast<uint8>((m_nOpcode >> 21) & 0x001F);
	auto immediate = static_cast<uint16>((m_nOpcode >> 0) & 0xFFFF);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[rs].nV[0]));
	m_codeGen->PushCst(static_cast<int16>(immediate));
	m_codeGen->Add();
	m_codeGen->PushCst(MIPS_PAGE_SIZE - accessSize);
	m_codeGen->And();
}

void CMIPSInstructionFactory::ComputeMemAccessPageRef()
{
	auto rs = static_cast<uint8>((m_nOpcode >> 21) & 0x001F);
	auto immediate = static_cast<int16>((m_nOpcode >> 0) & 0xFFFF);
	constexpr int32 pageSizeShift = Framework::GetPowerOf2(MIPS_PAGE_SIZE);
	m_codeGen->PushRelRef(offsetof(CMIPS, m_pageLookup));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[rs].nV[0]));
	m_codeGen->PushCst(immediate);
	m_codeGen->Add();
	m_codeGen->Srl(pageSizeShift); //Divide by MIPS_PAGE_SIZE
	m_codeGen->LoadRefFromRefIdx();
}

void CMIPSInstructionFactory::Branch(Jitter::CONDITION condition)
{
	uint16 nImmediate = (uint16)(m_nOpcode & 0xFFFF);

	m_codeGen->PushCst(MIPS_INVALID_PC);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	m_codeGen->BeginIf(condition);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst((m_instrPosition + 4) + CMIPS::GetBranch(nImmediate));
		m_codeGen->Add();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::BranchLikely(Jitter::CONDITION condition)
{
	uint16 nImmediate = (uint16)(m_nOpcode & 0xFFFF);

	m_codeGen->PushCst(MIPS_INVALID_PC);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	m_codeGen->BeginIf(condition);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst((m_instrPosition + 4) + CMIPS::GetBranch(nImmediate));
		m_codeGen->Add();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->Else();
	{
		m_codeGen->Goto(m_codeGen->GetLastBlockLabel());
	}
	m_codeGen->EndIf();
}

void CMIPSInstructionFactory::Illegal()
{
#ifdef _DEBUG
	m_codeGen->Break();
#endif
}
