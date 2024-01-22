#pragma once

#include "../BasicBlock.h"

class CVuBasicBlock : public CBasicBlock
{
public:
	CVuBasicBlock(CMIPS&, uint32, uint32, BLOCK_CATEGORY);
	virtual ~CVuBasicBlock() = default;

	bool IsLinkable() const;

protected:
	void CompileRange(CMipsJitter*) override;

private:
	struct INTEGER_BRANCH_DELAY_INFO
	{
		unsigned int regIndex = 0;
		uint32 saveRegAddress = MIPS_INVALID_PC;
		uint32 useRegAddress = MIPS_INVALID_PC;
	};

	static bool IsConditionalBranch(uint32);
	static bool IsNonConditionalBranch(uint32);

	typedef uint32 FmacRegWriteTimes[32][4];
	struct BlockFmacPipelineInfo
	{
		//State of the write times at the end of block
		FmacRegWriteTimes regWriteTimes = {};

		//Delays incurred during every instruction execution
		std::vector<uint32> stallDelays;

		//Pipe time at the end of block
		uint32 pipeTime = 0;
	};

	INTEGER_BRANCH_DELAY_INFO ComputeIntegerBranchDelayInfo(const std::vector<uint32>&) const;
	INTEGER_BRANCH_DELAY_INFO ComputeTrailingIntegerBranchDelayInfo(const std::vector<uint32>&) const;
	bool CheckIsSpecialIntegerLoop(unsigned int) const;
	void ComputeSkipFlagsHints(const std::vector<uint32>&, std::vector<uint32>&) const;
	BlockFmacPipelineInfo ComputeFmacStallDelays(uint32, uint32) const;
	static void EmitXgKick(CMipsJitter*);

	bool m_isLinkable = true;
};
