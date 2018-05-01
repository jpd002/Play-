#pragma once

#include "../BasicBlock.h"

class CVuBasicBlock : public CBasicBlock
{
public:
	CVuBasicBlock(CMIPS&, uint32, uint32);
	virtual ~CVuBasicBlock() = default;

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

	INTEGER_BRANCH_DELAY_INFO GetIntegerBranchDelayInfo(uint32) const;
	bool CheckIsSpecialIntegerLoop(uint32, unsigned int) const;
	static void EmitXgKick(CMipsJitter*);
};
