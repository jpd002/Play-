#ifndef _MIPSREFLECTION_H_
#define _MIPSREFLECTION_H_

#include "Types.h"

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
		const char*		sMnemonic;
		SUBTABLE*		pSubTable;
		void			(*pGetMnemonic)(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);
		void			(*pGetOperands)(INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		bool			(*pIsBranch)(INSTRUCTION*, CMIPS*, uint32);
		uint32			(*pGetEffectiveAddress)(INSTRUCTION*, CMIPS*, uint32, uint32);
	};

	INSTRUCTION*		DereferenceInstruction(SUBTABLE*, uint32);

	void				CopyMnemonic(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);
	void				SubTableMnemonic(INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);

	void				SubTableOperands(INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	bool				IsBranch(INSTRUCTION*, CMIPS*, uint32);
	bool				SubTableIsBranch(INSTRUCTION*, CMIPS*, uint32);

	uint32				SubTableEffAddr(INSTRUCTION*, CMIPS*, uint32, uint32);
};

#endif
