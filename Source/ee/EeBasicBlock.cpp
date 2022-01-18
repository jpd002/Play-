#include "EeBasicBlock.h"

void CEeBasicBlock::CompileEpilog(CMipsJitter* jitter)
{
	if(IsIdleLoopBlock())
	{
		jitter->PushCst(MIPS_EXCEPTION_IDLE);
		jitter->PullRel(offsetof(CMIPS, m_State.nHasException));
	}

	CBasicBlock::CompileEpilog(jitter);
}

bool CEeBasicBlock::IsIdleLoopBlock() const
{
	enum OP
	{
		OP_BEQ = 0x04,
		OP_BNE = 0x05,
		OP_SLTIU = 0x0B,
		OP_ANDI = 0x0C,
		OP_XORI = 0x0E,
		OP_LUI = 0x0F,
		OP_LQ = 0x1E,
		OP_LW = 0x23,
	};

	enum
	{
		OP_SPECIAL_SLT = 0x2A,
		OP_SPECIAL_SLTU = 0x2B,
	};

	uint32 endInstructionAddress = m_end - 4;
	uint32 endInstruction = m_context.m_pMemoryMap->GetWord(endInstructionAddress);

	//We need a branch at the end of the block
	auto branchType = m_context.m_pArch->IsInstructionBranch(&m_context, endInstructionAddress, endInstruction);
	if(branchType != MIPS_BRANCH_NORMAL) return false;

	//Check that the branch target is ourself
	uint32 branchTarget = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, endInstructionAddress, endInstruction);
	if(branchTarget != m_begin) return false;

	uint32 compareRs = 0;
	uint32 compareRt = 0;

	//Check what kind of branching instruction we have.
	{
		uint32 op = (endInstruction >> 26) & 0x3F;
		uint32 rt = (endInstruction >> 16) & 0x1F;
		uint32 rs = (endInstruction >> 21) & 0x1F;

		//We want a BEQ or BNE
		if((op != OP_BEQ) && (op != OP_BNE)) return false;

		compareRs = rs;
		compareRt = rt;
	}

	uint32 defState = 0; //Set of completely new definitions of registers within this block
	uint32 useState = 0; //Set of previous state usage within this block

	//Check all instructions inside to see if we can prove it's waiting for some kind of flag
	for(uint32 address = m_begin; address <= m_end; address += 4)
	{
		//Don't check branch instruction as we've checked it already
		if(address == endInstructionAddress) continue;

		uint32 inst = m_context.m_pMemoryMap->GetWord(address);
		if(inst == 0) continue;
		uint32 special = inst & 0x3F;
		uint32 rd = (inst >> 11) & 0x1F;
		uint32 rt = (inst >> 16) & 0x1F;
		uint32 rs = (inst >> 21) & 0x1F;
		uint32 op = (inst >> 26) & 0x3F;

		uint32 newDef = 0;
		uint32 newUse = 0;

		switch(op)
		{
		case 0x00:
			switch(special)
			{
			case OP_SPECIAL_SLT:
			case OP_SPECIAL_SLTU:
				newUse = (1 << rs) | (1 << rt);
				newDef = (1 << rd);
				break;
			default:
				//We don't know what this does, let's not take a chance
				return false;
			}
			break;
		case OP_LUI:
			newDef = (1 << rt);
			break;
		case OP_LW:
		case OP_LQ:
		case OP_SLTIU:
		case OP_XORI:
			newUse = (1 << rs);
			newDef = (1 << rt);
			break;
		default:
			//We don't know what this does, let's not take a chance
			return false;
		}

		//Bail if this defines any state that we previously used
		if(useState & newDef)
		{
			return false;
		}

		//Remove uses from defs within this block
		newUse &= ~defState;

		defState |= newDef;
		useState |= newUse;
	}

	//Just make sure that we've at least defined our comparision register
	bool compareRsDefined = defState & (1 << compareRs);
	bool compareRtDefined = defState & (1 << compareRt);

	assert(compareRsDefined || compareRtDefined);

	return true;
}
