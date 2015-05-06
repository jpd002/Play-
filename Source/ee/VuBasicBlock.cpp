#include "VuBasicBlock.h"
#include "MA_VU.h"
#include "offsetof_def.h"

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

		arch->SetRelativePipeTime(relativePipeTime);
		arch->CompileInstruction(addressHi, jitter, &m_context);

		if(savedReg != 0)
		{
			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));

			jitter->MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_PreUp));
			jitter->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
		}

		arch->CompileInstruction(addressLo, jitter, &m_context);

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
