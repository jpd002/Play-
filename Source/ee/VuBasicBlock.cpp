#include "VuBasicBlock.h"
#include "MA_VU.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"
#include "Vpu.h"

CVuBasicBlock::CVuBasicBlock(CMIPS& context, uint32 begin, uint32 end)
    : CBasicBlock(context, begin, end)
{
}

bool CVuBasicBlock::IsLinkable() const
{
	return m_isLinkable;
}

void CVuBasicBlock::CompileRange(CMipsJitter* jitter)
{
	CompileProlog(jitter);

	assert((m_begin & 0x07) == 0);
	assert(((m_end + 4) & 0x07) == 0);
	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);

	auto integerBranchDelayInfo = GetIntegerBranchDelayInfo();

	bool hasPendingXgKick = false;
	const auto clearPendingXgKick =
	    [&]() {
		    assert(hasPendingXgKick);
		    EmitXgKick(jitter);
		    hasPendingXgKick = false;
	    };

	BlockFmacPipelineInfo prevBlockFmacPipelineInfo;
	auto prevBlockWindow = GetPreviousBlockWindow();
	if((prevBlockWindow.second - prevBlockWindow.first) != 0)
	{
		prevBlockFmacPipelineInfo = ComputeFmacStallDelays(prevBlockWindow.first, prevBlockWindow.second);
		OffsetFmacWriteTimes(prevBlockFmacPipelineInfo);
	}

	auto fmacPipelineInfo = ComputeFmacStallDelays(m_begin, m_end, prevBlockFmacPipelineInfo.regWriteTimes);

	uint32 maxInstructions = ((m_end - m_begin) / 8) + 1;
	std::vector<uint32> hints;
	hints.resize(maxInstructions);

	ComputeSkipFlagsHints(fmacPipelineInfo.stallDelays, hints);

	uint32 relativePipeTime = 0;
	uint32 instructionIndex = 0;

	for(uint32 address = m_begin; address <= m_end; address += 8)
	{
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

		//No upper instruction writes to P
		assert(hiOps.syncP == false);

		//No upper instruction reads from P
		assert(hiOps.readP == false);

		bool loIsXgKick = (opcodeLo & ~(0x1F << 11)) == 0x800006FC;

		if(loOps.syncQ)
		{
			VUShared::FlushPipeline(VUShared::g_pipeInfoQ, jitter);
		}
		if(loOps.syncP)
		{
			VUShared::FlushPipeline(VUShared::g_pipeInfoP, jitter);
		}

		auto fmacStallDelay = fmacPipelineInfo.stallDelays[instructionIndex];
		relativePipeTime += fmacStallDelay;

		if(hiOps.readQ)
		{
			VUShared::CheckPipeline(VUShared::g_pipeInfoQ, jitter, relativePipeTime);
		}
		if(loOps.readP)
		{
			VUShared::CheckPipeline(VUShared::g_pipeInfoP, jitter, relativePipeTime);
		}

		uint8 savedReg = 0;

		if(hiOps.writeF != 0)
		{
			assert(hiOps.writeF != loOps.writeF);
			if(
			    (hiOps.writeF == loOps.readF0) ||
			    (hiOps.writeF == loOps.readF1))
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

		uint32 compileHints = hints[instructionIndex];
		arch->SetRelativePipeTime(relativePipeTime, compileHints);
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

		//If there's a pending XGKICK and the current lower instruction is
		//an XGKICK, make sure we flush the pending one first
		if(loIsXgKick && hasPendingXgKick)
		{
			clearPendingXgKick();
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
		//Adjust pipeTime
		relativePipeTime++;
		instructionIndex++;

		//Handle some branch in delay slot situation (Star Ocean 3):
		//B   $label1
		//Bxx $label2
		if((address == (m_end - 4)) && IsConditionalBranch(opcodeLo))
		{
			//Disable block linking because targets will be wrong
			m_isLinkable = false;

			uint32 branchOpcodeAddr = address - 8;
			assert(branchOpcodeAddr >= m_begin);
			uint32 branchOpcodeLo = m_context.m_pMemoryMap->GetInstruction(branchOpcodeAddr);
			if(IsNonConditionalBranch(branchOpcodeLo))
			{
				//We need to compile the instruction at the branch target because it will be executed
				//before the branch is taken
				uint32 branchTgtAddress = branchOpcodeAddr + VUShared::GetBranch(branchOpcodeLo & 0x7FF) + 8;
				arch->CompileInstruction(branchTgtAddress, jitter, &m_context);
			}
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
		jitter->PushRel(offsetof(CMIPS, m_State.pipeTime));
		jitter->PushCst(relativePipeTime);
		jitter->Add();
		jitter->PullRel(offsetof(CMIPS, m_State.pipeTime));
	}

	CompileEpilog(jitter);
}

bool CVuBasicBlock::IsConditionalBranch(uint32 opcodeLo)
{
	//Conditional branches are in the contiguous opcode range 0x28 -> 0x2F inclusive
	uint32 id = (opcodeLo >> 25) & 0x7F;
	return (id >= 0x28) && (id < 0x30);
}

bool CVuBasicBlock::IsNonConditionalBranch(uint32 opcodeLo)
{
	uint32 id = (opcodeLo >> 25) & 0x7F;
	return (id == 0x20);
}

CVuBasicBlock::INTEGER_BRANCH_DELAY_INFO CVuBasicBlock::GetIntegerBranchDelayInfo() const
{
	// Test if the block ends with a conditional branch instruction where the condition variable has been
	// set in the prior instruction.
	// In this case, the pipeline shortcut fails and we need to use the value from 4 instructions previous.
	// If the relevant set instruction is not part of this block, use initial value of the integer register.

	INTEGER_BRANCH_DELAY_INFO result;
	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);
	uint32 adjustedEnd = m_end - 4;

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
			    (branchLoOps.readI1 == priorLoOps.writeI))
			{
				//Check if our block is a "special" loop. Disable delayed integer processing if it's the case
				//TODO: Handle that case better
				bool isSpecialLoop = CheckIsSpecialIntegerLoop(priorLoOps.writeI);
				if(!isSpecialLoop)
				{
					// we need to use the value of intReg 4 steps prior or use initial value.
					result.regIndex = priorLoOps.writeI;
					result.saveRegAddress = std::max(adjustedEnd - 5 * 8, m_begin);
					result.useRegAddress = adjustedEnd - 8;
				}
			}
		}
	}

	return result;
}

bool CVuBasicBlock::CheckIsSpecialIntegerLoop(unsigned int regI) const
{
	//This checks for a pattern where all instructions within a block
	//modifies an integer register except for one branch instruction that
	//tests that integer register
	//Required by BGDA that has that kind of loop inside its VU microcode

	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);
	uint32 length = (m_end - m_begin) / 8;
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
	jitter->PushCst(CVpu::VU_ADDR_XGKICK);

	jitter->Call(reinterpret_cast<void*>(&MemoryUtils_SetWordProxy), 3, false);
}

void CVuBasicBlock::ComputeSkipFlagsHints(const std::vector<uint32>& fmacStallDelays, std::vector<uint32>& hints) const
{
	static const uint32 g_undefinedMACflagsResult = -1;

	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);

	uint32 maxInstructions = static_cast<uint32>(hints.size());

	uint32 maxPipeTime = maxInstructions;
	for(const auto& fmacStallDelay : fmacStallDelays)
		maxPipeTime += fmacStallDelay;

	//Take into account the instructions that come after this block (up to 4 cycles later)
	uint32 extendedMaxPipeTime = maxPipeTime + VUShared::LATENCY_MAC;

	std::vector<uint32> flagsResults;
	flagsResults.resize(extendedMaxPipeTime);
	std::fill(flagsResults.begin(), flagsResults.end(), g_undefinedMACflagsResult);

	std::vector<bool> resultUsed;
	resultUsed.resize(maxInstructions);

	uint32 relativePipeTime = 0;
	for(uint32 address = m_begin; address <= m_end; address += 8)
	{
		uint32 instructionIndex = (address - m_begin) / 8;
		assert(instructionIndex < maxInstructions);

		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		auto loOps = arch->GetAffectedOperands(&m_context, addressLo, opcodeLo);
		auto hiOps = arch->GetAffectedOperands(&m_context, addressHi, opcodeHi);

		relativePipeTime += fmacStallDelays[instructionIndex];

		if(hiOps.writeMACflags)
		{
			//Make this result available
			std::fill(
			    flagsResults.begin() + relativePipeTime + VUShared::LATENCY_MAC,
			    flagsResults.end(), instructionIndex);
		}

		if(loOps.readMACflags)
		{
			uint32 pipeTimeForResult = flagsResults[relativePipeTime];
			if(pipeTimeForResult != g_undefinedMACflagsResult)
			{
				resultUsed[pipeTimeForResult] = true;
			}
		}

		relativePipeTime++;
	}

	//Simulate usage from outside our block
	for(uint32 relativePipeTime = maxPipeTime; relativePipeTime < extendedMaxPipeTime; relativePipeTime++)
	{
		uint32 pipeTimeForResult = flagsResults[relativePipeTime];
		if(pipeTimeForResult != g_undefinedMACflagsResult)
		{
			resultUsed[pipeTimeForResult] = true;
		}
	}

	//Flag unused results
	for(uint32 instructionIndex = 0; instructionIndex < maxInstructions; instructionIndex++)
	{
		bool used = resultUsed[instructionIndex];
		if(!used)
		{
			hints[instructionIndex] |= VUShared::COMPILEHINT_SKIPFMACUPDATE;
		}
	}
}

CVuBasicBlock::BlockFmacPipelineInfo CVuBasicBlock::ComputeFmacStallDelays(uint32 begin, uint32 end, FmacRegWriteTimes initWriteFTime) const
{
	auto arch = static_cast<CMA_VU*>(m_context.m_pArch);

	assert((begin & 0x07) == 0);
	assert(((end + 4) & 0x07) == 0);
	uint32 maxInstructions = ((end - begin) / 8) + 1;

	std::vector<uint32> fmacStallDelays;
	fmacStallDelays.resize(maxInstructions);

	uint32 relativePipeTime = 0;
	FmacRegWriteTimes writeFTime = {};
	if(initWriteFTime != nullptr)
	{
		memcpy(writeFTime, initWriteFTime, sizeof(writeFTime));
	}

	auto adjustPipeTime =
	    [&writeFTime](uint32 pipeTime, uint32 dest, uint32 regIndex) {
		    if(regIndex == 0) return pipeTime;
		    for(unsigned int i = 0; i < 4; i++)
		    {
			    if(dest & (1 << i))
			    {
				    pipeTime = std::max<uint32>(pipeTime, writeFTime[regIndex][i]);
			    }
		    }
		    return pipeTime;
	    };

	for(uint32 address = begin; address <= end; address += 8)
	{
		uint32 instructionIndex = (address - begin) / 8;
		assert(instructionIndex < maxInstructions);

		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		auto loOps = arch->GetAffectedOperands(&m_context, addressLo, opcodeLo);
		auto hiOps = arch->GetAffectedOperands(&m_context, addressHi, opcodeHi);

		uint32 loDest = (opcodeLo >> 21) & 0xF;
		uint32 hiDest = (opcodeHi >> 21) & 0xF;

		//Instruction executes...

		relativePipeTime++;
		uint32 prevRelativePipeTime = relativePipeTime;

		relativePipeTime = adjustPipeTime(relativePipeTime, loOps.readElemF0, loOps.readF0);
		relativePipeTime = adjustPipeTime(relativePipeTime, loOps.readElemF1, loOps.readF1);
		relativePipeTime = adjustPipeTime(relativePipeTime, hiOps.readElemF0, hiOps.readF0);
		relativePipeTime = adjustPipeTime(relativePipeTime, hiOps.readElemF1, hiOps.readF1);

		if(prevRelativePipeTime != relativePipeTime)
		{
			//We got a stall, sync
			assert(relativePipeTime >= prevRelativePipeTime);
			uint32 diff = relativePipeTime - prevRelativePipeTime;
			fmacStallDelays[instructionIndex] = diff;
		}

		if(loOps.writeF != 0)
		{
			assert(loOps.writeF < 32);
			for(uint32 i = 0; i < 4; i++)
			{
				if(loDest & (1 << i))
				{
					writeFTime[loOps.writeF][i] = relativePipeTime + VUShared::LATENCY_MAC;
				}
			}
		}

		if(hiOps.writeF != 0)
		{
			assert(hiOps.writeF < 32);
			for(uint32 i = 0; i < 4; i++)
			{
				if(hiDest & (1 << i))
				{
					writeFTime[hiOps.writeF][i] = relativePipeTime + VUShared::LATENCY_MAC;
				}
			}
		}
	}

	//TODO: Check that we don't have unconditional branches?

	BlockFmacPipelineInfo result;
	result.pipeTime = relativePipeTime;
	result.stallDelays = fmacStallDelays;
	memcpy(result.regWriteTimes, writeFTime, sizeof(writeFTime));
	return result;
}

std::pair<uint32, uint32> CVuBasicBlock::GetPreviousBlockWindow() const
{
	//3 previous instructions
	static const uint32 windowSize = (3 * 8);
	auto result = std::make_pair(0U, 0U);
	if(m_begin >= windowSize)
	{
		//TODO: Check for unconditional jumps
		result = std::make_pair(m_begin - windowSize, m_begin - 4);
		assert((result.second + 4) == m_begin);
	}
	assert(result.second >= result.first);
	return result;
}

void CVuBasicBlock::OffsetFmacWriteTimes(BlockFmacPipelineInfo& pipelineInfo)
{
	auto& writeFTime = pipelineInfo.regWriteTimes;

	//Resync all write times
	for(uint32 reg = 0; reg < 32; reg++)
	{
		for(uint32 elem = 0; elem < 4; elem++)
		{
			if(writeFTime[reg][elem] >= pipelineInfo.pipeTime)
			{
				writeFTime[reg][elem] -= pipelineInfo.pipeTime;
			}
		}
	}
}
