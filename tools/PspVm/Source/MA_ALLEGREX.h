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
		MAX_SPECIAL3_OPS = 0x40,
	};

	using CMIPSInstructionFactory::Illegal;

	InstructionFunction					m_pOpSpecial3[MAX_SPECIAL3_OPS];

	void								SetupReflectionTables();

	static void							ReflOpIns(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	MIPSReflection::SUBTABLE			m_ReflSpecial3Table;
	MIPSReflection::INSTRUCTION			m_ReflSpecial3[64];

private:
	void								SPECIAL3();

	//SPECIAL3
	void								INS();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflSpecial3[64];

};

#endif
