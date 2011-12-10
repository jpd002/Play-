#ifndef _MIPSINSTRUCTIONFACTORY_H_
#define _MIPSINSTRUCTIONFACTORY_H_

#include "Types.h"
#include "MipsJitter.h"

class CMIPS;

enum MIPS_REGSIZE
{
	MIPS_REGSIZE_32		= 0,
	MIPS_REGSIZE_64		= 1,
};

enum MIPS_BRANCH_TYPE
{
	MIPS_BRANCH_NONE	= 0,
	MIPS_BRANCH_NORMAL	= 1,
	MIPS_BRANCH_NODELAY	= 2,
};

class CMIPSInstructionFactory
{
public:
							CMIPSInstructionFactory(MIPS_REGSIZE);
	virtual					~CMIPSInstructionFactory();
	virtual void			CompileInstruction(uint32, CMipsJitter*, CMIPS*) = 0;

protected:
	void					ComputeMemAccessAddr();
	void					Branch(Jitter::CONDITION);
	void					BranchLikely(Jitter::CONDITION);

	void					Illegal();
	void					SetupQuickVariables(uint32, CMipsJitter*, CMIPS*);

    CMipsJitter*			m_codeGen;
    CMIPS*					m_pCtx;
    uint32					m_nOpcode;
    uint32					m_nAddress;
	MIPS_REGSIZE			m_regSize;
};

#endif
