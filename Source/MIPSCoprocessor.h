#ifndef _MIPSCOPROCESSOR_H_
#define _MIPSCOPROCESSOR_H_

#include "MIPSInstructionFactory.h"

class CMIPSCoprocessor : public CMIPSInstructionFactory
{
public:
	CMIPSCoprocessor(MIPS_REGSIZE);
	virtual ~CMIPSCoprocessor();
	virtual void GetInstruction(uint32, char*) = 0;
	virtual void GetArguments(uint32, uint32, char*) = 0;
	virtual uint32 GetEffectiveAddress(uint32, uint32) = 0;
	virtual MIPS_BRANCH_TYPE IsBranch(uint32) = 0;
};

#endif
