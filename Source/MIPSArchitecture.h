#pragma once

#include "MIPSInstructionFactory.h"

class CMIPSArchitecture : public CMIPSInstructionFactory
{
public:
								CMIPSArchitecture(MIPS_REGSIZE);
	virtual						~CMIPSArchitecture() = default;
	virtual void				GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int) = 0;
	virtual void				GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int) = 0;
	virtual MIPS_BRANCH_TYPE	IsInstructionBranch(CMIPS*, uint32, uint32)	= 0;
	virtual uint32				GetInstructionEffectiveAddress(CMIPS*, uint32, uint32) = 0;
};
