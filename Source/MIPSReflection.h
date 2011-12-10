#ifndef _MIPSREFLECTION_H_
#define _MIPSREFLECTION_H_

#include "Types.h"
#include "MIPSArchitecture.h"

class CMIPS;

namespace MIPSReflection
{
	struct INSTRUCTION;

	struct SUBTABLE
	{
		uint32			nShift;
		uint32			nMask;
		INSTRUCTION*	pTable;	
	};

	struct INSTRUCTION
	{
		const char*			sMnemonic;
		SUBTABLE*			pSubTable;
		void				(*pGetMnemonic)(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);
		void				(*pGetOperands)(INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		MIPS_BRANCH_TYPE	(*pIsBranch)(INSTRUCTION*, CMIPS*, uint32);
		uint32				(*pGetEffectiveAddress)(INSTRUCTION*, CMIPS*, uint32, uint32);
	};

	INSTRUCTION*			DereferenceInstruction(SUBTABLE*, uint32);

	void					CopyMnemonic(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);
	void					SubTableMnemonic(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);

	void					SubTableOperands(INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	MIPS_BRANCH_TYPE		IsBranch(INSTRUCTION*, CMIPS*, uint32);
	MIPS_BRANCH_TYPE		IsNoDelayBranch(INSTRUCTION*, CMIPS*, uint32);
	MIPS_BRANCH_TYPE		SubTableIsBranch(INSTRUCTION*, CMIPS*, uint32);

	uint32					SubTableEffAddr(INSTRUCTION*, CMIPS*, uint32, uint32);
};

#endif
