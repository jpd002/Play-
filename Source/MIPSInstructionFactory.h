#ifndef _MIPSINSTRUCTIONFACTORY_H_
#define _MIPSINSTRUCTIONFACTORY_H_

#include "Types.h"

class CMIPS;
class CCodeGen;

enum MIPS_REGSIZE
{
	MIPS_REGSIZE_32		= 0,
	MIPS_REGSIZE_64		= 1,
};

class CMIPSInstructionFactory
{
public:
							CMIPSInstructionFactory(MIPS_REGSIZE);
	virtual					~CMIPSInstructionFactory();
	virtual void			CompileInstruction(uint32, CCodeGen*, CMIPS*) = 0;

protected:
	void					ComputeMemAccessAddr();
	void					Branch(bool);
	void					BranchLikely(bool);

	void					Illegal();
	void					SetupQuickVariables(uint32, CCodeGen*, CMIPS*);

    CCodeGen*				m_codeGen;
    CMIPS*					m_pCtx;
    uint32					m_nOpcode;
    uint32					m_nAddress;
	MIPS_REGSIZE			m_regSize;
};

#endif
