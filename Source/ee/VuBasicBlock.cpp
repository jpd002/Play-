#include "VuBasicBlock.h"
#include "MA_VU.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"
#include "Vpu.h"

CVuBasicBlock::CVuBasicBlock(CMIPS& context, uint32 begin, uint32 end)
: CBasicBlock(context, begin, end)
{

}

void CVuBasicBlock::CompileRange(CMipsJitter* jitter)
{
	assert((m_begin & 0x07) == 0);
	assert(((m_end + 4) & 0x07) == 0);
	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);

	uint32 fixedEnd = m_end;
	bool needsPcAdjust = false;

	//Make sure the delay slot instruction is present in the block.
	//CVuExecutor can sometimes cut the blocks in a way that removes the delay slot instruction for branches.
	{
		uint32 addressLo = fixedEnd - 4;
		uint32 addressHi = fixedEnd - 0;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		//Check for LOI
		if((opcodeHi & 0x80000000) == 0)
		{
			auto branchType = arch->IsInstructionBranch(&m_context, addressLo, opcodeLo);
			if(branchType == MIPS_BRANCH_NORMAL)
			{
				fixedEnd += 8;
				needsPcAdjust = true;
			}
		}
	}

	auto integerBranchDelayInfo = GetIntegerBranchDelayInfo(fixedEnd);

	bool hasPendingXgKick = false;
	const auto clearPendingXgKick =
		[&]()
		{
			assert(hasPendingXgKick);
			EmitXgKick(jitter);
			hasPendingXgKick = false;
		};

	for(uint32 address = m_begin; address <= fixedEnd; address += 8)
	{
		uint32 relativePipeTime = (address - m_begin) / 8;

		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		auto loOps = arch->GetAffectedOperands(&m_context, addressLo, opcodeLo);
		auto hiOps = arch->GetAffectedOperands(&m_context, addressHi, opcodeHi);

		//No upper instruction writes to Q
		assert(hiOps.syncQ == false);
		
		//No lower instruction reads Q
		assert(loOps.readQ == false);

		bool loIsXgKick = (opcodeLo & ~(0x1F << 11)) == 0x800006FC;

		if(loOps.syncQ)
		{
			VUShared::FlushPipeline(VUShared::g_pipeInfoQ, jitter);
		}

		if(hiOps.readQ)
		{
			VUShared::CheckPipeline(VUShared::g_pipeInfoQ, jitter, relativePipeTime);
		}

		uint8 savedReg = 0;

		if(hiOps.writeF != 0)
		{
			assert(hiOps.writeF != loOps.writeF);
			if(
				(hiOps.writeF == loOps.readF0) ||
				(hiOps.writeF == loOps.readF1)
				)
			{
				savedReg = hiOps.writeF;
				jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
				jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2VF_PreUp));
			}
		}

		if(address == integerBranchDelayInfo.saveRegAddress)
		{
			// grab the value of the delayed reg to use in the conditional branch later
			jitter->PushRel(offsetof(CMIPS, m_State.nCOP2VI[integerBranchDelayInfo.regIndex]));
			jitter->PullRel(offsetof(CMIPS, m_State.savedIntReg));
		}

		arch->SetRelativePipeTime(relativePipeTime);
		arch->CompileInstruction(addressHi, jitter, &m_context);

		if(savedReg != 0)
		{
			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));

			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_PreUp));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
		}

		if(address == integerBranchDelayInfo.useRegAddress)
		{
			// set the target from the saved value
			jitter->PushRel(offsetof(CMIPS, m_State.nCOP2VI[integerBranchDelayInfo.regIndex]));
			jitter->PullRel(offsetof(CMIPS, m_State.savedIntRegTemp));

			jitter->PushRel(offsetof(CMIPS, m_State.savedIntReg));
			jitter->PullRel(offsetof(CMIPS, m_State.nCOP2VI[integerBranchDelayInfo.regIndex]));
		}

		arch->CompileInstruction(addressLo, jitter, &m_context);

		if(address == integerBranchDelayInfo.useRegAddress)
		{
			// put the target value back
			jitter->PushRel(offsetof(CMIPS, m_State.savedIntRegTemp));
			jitter->PullRel(offsetof(CMIPS, m_State.nCOP2VI[integerBranchDelayInfo.regIndex]));
		}

		if(savedReg != 0)
		{
			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
		}

		if(hasPendingXgKick)
		{
			clearPendingXgKick();
		}

		if(loIsXgKick)
		{
			assert(!hasPendingXgKick);
			hasPendingXgKick = true;
		}

		//Sanity check
		assert(jitter->IsStackEmpty());
	}

	if(hasPendingXgKick)
	{
		clearPendingXgKick();
	}

	assert(!hasPendingXgKick);

	//Increment pipeTime
	{
		uint32 timeInc = ((fixedEnd - m_begin) / 8) + 1;
		jitter->PushRel(offsetof(CMIPS, m_State.pipeTime));
		jitter->PushCst(timeInc);
		jitter->Add();
		jitter->PullRel(offsetof(CMIPS, m_State.pipeTime));
	}

	//Adjust PC to make sure we don't execute the delay slot at the next block
	if(needsPcAdjust)
	{
		jitter->PushCst(MIPS_INVALID_PC);
		jitter->PushRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		jitter->BeginIf(Jitter::CONDITION_EQ);
		{
			jitter->PushCst(fixedEnd + 0x4);
			jitter->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
		}
		jitter->EndIf();
	}
}

bool CVuBasicBlock::IsConditionalBranch(uint32 opcodeLo)
{
	//Conditional branches are in the contiguous opcode range 0x28 -> 0x2F inclusive
	uint32 id = (opcodeLo >> 25) & 0x7F;
	return (id >= 0x28) && (id < 0x30);
}

CVuBasicBlock::INTEGER_BRANCH_DELAY_INFO CVuBasicBlock::GetIntegerBranchDelayInfo(uint32 fixedEnd) const
{
	// Test if the block ends with a conditional branch instruction where the condition variable has been
	// set in the prior instruction.
	// In this case, the pipeline shortcut fails and we need to use the value from 4 instructions previous.
	// If the relevant set instruction is not part of this block, use initial value of the integer register.

	INTEGER_BRANCH_DELAY_INFO result;
	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);
	uint32 adjustedEnd = fixedEnd - 4;

	// Check if we have a conditional branch instruction.
	uint32 branchOpcodeAddr = adjustedEnd - 8;
	uint32 branchOpcodeLo = m_context.m_pMemoryMap->GetInstruction(branchOpcodeAddr);
	if(IsConditionalBranch(branchOpcodeLo))
	{
		// We have a conditional branch instruction. Now we need to check that the condition register is not written
		// by the previous instruction.
		uint32 priorOpcodeAddr = adjustedEnd - 16;
		uint32 priorOpcodeLo = m_context.m_pMemoryMap->GetInstruction(priorOpcodeAddr);

		auto priorLoOps = arch->GetAffectedOperands(&m_context, priorOpcodeAddr, priorOpcodeLo);
		if((priorLoOps.writeI != 0) && !priorLoOps.branchValue)
		{
			auto branchLoOps = arch->GetAffectedOperands(&m_context, branchOpcodeAddr, branchOpcodeLo);
			if(
				(branchLoOps.readI0 == priorLoOps.writeI) || 
				(branchLoOps.readI1 == priorLoOps.writeI)
				)
			{
				//Check if our block is a "special" loop. Disable delayed integer processing if it's the case
				//TODO: Handle that case better
				bool isSpecialLoop = CheckIsSpecialIntegerLoop(fixedEnd, priorLoOps.writeI);
				if(!isSpecialLoop)
				{
					// we need to use the value of intReg 4 steps prior or use initial value.
					result.regIndex       = priorLoOps.writeI;
					result.saveRegAddress = std::max(adjustedEnd - 5 * 8, m_begin);
					result.useRegAddress  = adjustedEnd - 8;
				}
			}
		}
	}

	return result;
}

bool CVuBasicBlock::CheckIsSpecialIntegerLoop(uint32 fixedEnd, unsigned int regI) const
{
	//This checks for a pattern where all instructions within a block
	//modifies an integer register except for one branch instruction that
	//tests that integer register
	//Required by BGDA that has that kind of loop inside its VU microcode

	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);
	uint32 length = (fixedEnd - m_begin) / 8;
	if(length != 4) return false;
	for(uint32 index = 0; index <= length; index++)
	{
		uint32 address = m_begin + (index * 8);
		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(address);
		if(index == (length - 1))
		{
			assert(IsConditionalBranch(opcodeLo));
			uint32 branchTarget = arch->GetInstructionEffectiveAddress(&m_context, address, opcodeLo);
			if(branchTarget != m_begin) return false;
		}
		else
		{
			auto loOps = arch->GetAffectedOperands(&m_context, address, opcodeLo);
			if(loOps.writeI != regI) return false;
		}
	}

	return true;
}

void CVuBasicBlock::EmitXgKick(CMipsJitter* jitter)
{
	//Push context
	jitter->PushCtx();

	//Push value
	jitter->PushRel(offsetof(CMIPS, m_State.xgkickAddress));

	//Compute Address
	jitter->PushCst(CVpu::VU_XGKICK);

	jitter->Call(reinterpret_cast<void*>(&MemoryUtils_SetWordProxy), 3, false);
}
