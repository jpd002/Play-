#ifndef _MIPS_ARCHITECTURE_H_
#define _MIPS_ARCHITECTURE_H_

#include "MIPSInstructionFactory.h"

class CMIPSArchitecture : public CMIPSInstructionFactory
{
public:
						CMIPSArchitecture(MIPS_REGSIZE);
	virtual				~CMIPSArchitecture();
	virtual void		GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int) = 0;
	virtual void		GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int) = 0;
	virtual bool		IsInstructionBranch(CMIPS*, uint32, uint32)	= 0;
	virtual uint32		GetInstructionEffectiveAddress(CMIPS*, uint32, uint32) = 0;
};

#endif
