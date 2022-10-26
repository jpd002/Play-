#pragma once

#include "Types.h"
#include "MipsJitter.h"

class CMIPS;

enum MIPS_REGSIZE
{
	MIPS_REGSIZE_32 = 0,
	MIPS_REGSIZE_64 = 1,
};

enum MIPS_BRANCH_TYPE
{
	MIPS_BRANCH_NONE = 0,
	MIPS_BRANCH_NORMAL = 1,
	MIPS_BRANCH_NODELAY = 2,
};

class CMIPSInstructionFactory
{
public:
	CMIPSInstructionFactory(MIPS_REGSIZE);
	virtual ~CMIPSInstructionFactory() = default;
	virtual void CompileInstruction(uint32, CMipsJitter*, CMIPS*, uint32) = 0;
	void Illegal();

protected:
	void ComputeMemAccessAddr();
	void ComputeMemAccessAddrNoXlat();
	void ComputeMemAccessRef(uint32);
	void ComputeMemAccessPageRef();

	void CheckTLBExceptions(bool);
	void Branch(Jitter::CONDITION);
	void BranchLikely(Jitter::CONDITION);

	void SetupQuickVariables(uint32, CMipsJitter*, CMIPS*, uint32);

	CMipsJitter* m_codeGen = nullptr;
	CMIPS* m_pCtx = nullptr;
	uint32 m_nOpcode = 0;
	uint32 m_nAddress = 0;
	uint32 m_instrPosition = 0;
	MIPS_REGSIZE m_regSize;
};
