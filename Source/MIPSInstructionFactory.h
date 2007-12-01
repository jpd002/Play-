#ifndef _MIPSINSTRUCTIONFACTORY_H_
#define _MIPSINSTRUCTIONFACTORY_H_

#include "Types.h"

class CMIPS;
class CMipsCodeGen;
class CCacheBlock;

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
	virtual void			CompileInstruction(uint32, CCacheBlock*, CMIPS*, bool) = 0;

protected:
	static void				SignExtendTop32(unsigned int);
	static void				ComputeMemAccessAddr();

	static void				Branch(bool);
	static void				BranchLikely(bool);

	static void				ComputeMemAccessAddrEx();
	static void				BranchEx(bool);

	static void				Illegal();
	static void				SetupQuickVariables(uint32, CCacheBlock*, CMIPS*);

	static CCacheBlock*		m_pB;
    static CMipsCodeGen*    m_codeGen;
	static CMIPS*			m_pCtx;
	static uint32			m_nOpcode;
	static uint32			m_nAddress;
};

#endif