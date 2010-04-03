#ifndef _MA_ALLEGREX_H_
#define _MA_ALLEGREX_H_

#include "MA_MIPSIV.h"

class CMA_ALLEGREX : public CMA_MIPSIV
{
public:
										CMA_ALLEGREX();
	virtual                             ~CMA_ALLEGREX();

protected:
	enum
	{
		MAX_SPECIAL3_OPS	= 0x40,
		MAX_BSHFL_OPS		= 0x20,
	};

	InstructionFunction					m_pOpSpecial3[MAX_SPECIAL3_OPS];
	InstructionFunction					m_pOpBshfl[MAX_BSHFL_OPS];

	void								SetupReflectionTables();

	static void							ReflOpExt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpIns(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRdRt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	MIPSReflection::SUBTABLE			m_ReflSpecial3Table;
	MIPSReflection::SUBTABLE			m_ReflBshflTable;

	MIPSReflection::INSTRUCTION			m_ReflSpecial3[64];
	MIPSReflection::INSTRUCTION			m_ReflBshfl[32];

private:
	void								SPECIAL3();

	//SPECIAL
	void								MAX();
	void								MIN();

	//SPECIAL3
	void								EXT();
	void								INS();
	void								BSHFL();

	//BSHFL
	void								SEB();
	void								SEH();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflSpecial3[64];
	static MIPSReflection::INSTRUCTION	m_cReflBshfl[32];

};

#endif
