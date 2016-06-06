#include "VuBasicBlock.h"
#include "MA_VU.h"
#include "offsetof_def.h"

#pragma optimize("", off)

CVuBasicBlock::CVuBasicBlock(CMIPS& context, uint32 begin, uint32 end)
: CBasicBlock(context, begin, end)
{

}

CVuBasicBlock::~CVuBasicBlock()
{

}

void CVuBasicBlock::CompileRange(CMipsJitter* jitter)
{
	assert((m_begin & 0x07) == 0);
	assert(((m_end + 4) & 0x07) == 0);
	CMA_VU* arch = static_cast<CMA_VU*>(m_context.m_pArch);

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

	int delayedReg = 0;
	int delayedRegTime = -1;
	int delayedRegTargetTime = -1;
	int adjustedEnd = fixedEnd - 4;
	{
		// Test if the block ends with a conditional branch instruction where the condition variable has been
		// set in the prior instruction.
		// In this case, the pipeline shortcut fails and we need to use the value from 4 instructions previous.
		int length = (fixedEnd - m_begin) >> 3;
		if (length > 2)
		{
			// Check if we have a conditional branch instruction. Luckily these populate the contiguous opcode range 0x28 -> 0x2F inclusive
			uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(adjustedEnd - 8);
			uint32 id = (opcodeLo >> 25) & 0x7f;
			if (id >= 0x28 && id < 0x30)
			{
				// We have a conditional branch instruction. Now we need to check that the condition register is not written
				// by the previous instruction.
				int priorOpcodeAddr = adjustedEnd - 16;
				uint32 priorOpcodeLo = m_context.m_pMemoryMap->GetInstruction(priorOpcodeAddr);

				VUShared::OPERANDSET loOps = arch->GetAffectedOperands(&m_context, priorOpcodeAddr, priorOpcodeLo);
				if (loOps.writeI != 0) {
					uint8  is = static_cast<uint8> ((opcodeLo >> 11) & 0x001F);
					uint8  it = static_cast<uint8> ((opcodeLo >> 16) & 0x001F);
					if (is == loOps.writeI || it == loOps.writeI) {
						// argh - we need to use the value of incReg 4 steps prior.
						delayedReg = loOps.writeI;
						delayedRegTime = adjustedEnd - 5 * 8;
						delayedRegTargetTime = adjustedEnd - 8;
					}
				}			
			}
		}
	}

	for(uint32 address = m_begin; address <= fixedEnd; address += 8)
	{
		uint32 relativePipeTime = (address - m_begin) / 8;

		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		VUShared::OPERANDSET loOps = arch->GetAffectedOperands(&m_context, addressLo, opcodeLo);
		VUShared::OPERANDSET hiOps = arch->GetAffectedOperands(&m_context, addressHi, opcodeHi);

		//No upper instruction writes to Q
		assert(hiOps.syncQ == false);
		
		//No lower instruction reads Q
		assert(loOps.readQ == false);

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

		if (address == delayedRegTime) {
			// grab the value of the delayed reg to use in the conditional branch later
			jitter->PushRel(offsetof(CMIPS, m_State.nCOP2VI[delayedReg]));
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

		if (address == delayedRegTargetTime) {
			// set the target from the saved value
			jitter->PushRel(offsetof(CMIPS, m_State.nCOP2VI[delayedReg]));
			jitter->PullRel(offsetof(CMIPS, m_State.savedIntRegTemp));

			jitter->PushRel(offsetof(CMIPS, m_State.savedIntReg));
			jitter->PullRel(offsetof(CMIPS, m_State.nCOP2VI[delayedReg]));
		}

		arch->CompileInstruction(addressLo, jitter, &m_context);

		if (address == delayedRegTargetTime) {
			// put the target value back
			jitter->PushRel(offsetof(CMIPS, m_State.savedIntRegTemp));
			jitter->PullRel(offsetof(CMIPS, m_State.nCOP2VI[delayedReg]));
		}

		if(savedReg != 0)
		{
			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
		}

		//Sanity check
		assert(jitter->IsStackEmpty());
	}

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
