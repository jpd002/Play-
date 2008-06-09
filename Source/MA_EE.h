#ifndef _MA_EE_H_
#define _MA_EE_H_

#include "MA_MIPSIV.h"

class CMA_EE : public CMA_MIPSIV
{
public:
										CMA_EE();
										~CMA_EE();

protected:
	void								SetupReflectionTables();

	static void							(*m_pOpMmi0[0x20])();
	static void							(*m_pOpMmi1[0x20])();
	static void							(*m_pOpMmi2[0x20])();
	static void							(*m_pOpMmi3[0x20])();

	static void							ReflOpRdRt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsImm(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	MIPSReflection::INSTRUCTION			m_ReflMmi[64];
	MIPSReflection::INSTRUCTION			m_ReflMmi0[32];
	MIPSReflection::INSTRUCTION			m_ReflMmi1[32];
	MIPSReflection::INSTRUCTION			m_ReflMmi2[32];
	MIPSReflection::INSTRUCTION			m_ReflMmi3[32];

	MIPSReflection::SUBTABLE			m_ReflMmiTable;
	MIPSReflection::SUBTABLE			m_ReflMmi0Table;
	MIPSReflection::SUBTABLE			m_ReflMmi1Table;
	MIPSReflection::SUBTABLE			m_ReflMmi2Table;
	MIPSReflection::SUBTABLE			m_ReflMmi3Table;

private:

	static void							PushVector(unsigned int);
	static void							PullVector(unsigned int);

	//General
	static void							LQ();
	static void							SQ();

	//Special
	static void							REEXCPT();

	//RegImm
	static void							MTSAB();
	static void							MTSAH();

	//Special2
	static void							MADD();
	static void							PLZCW();
	static void							MMI0();
	static void							MMI2();
	static void							MFHI1();
	static void							MTHI1();
	static void							MFLO1();
	static void							MTLO1();
	static void							MULT1();
	static void							MULTU1();
	static void							DIV1();
	static void							DIVU1();
	static void							MMI1();
	static void							MMI3();
	static void							PSLLH();
	static void							PSRLH();
	static void							PSRAH();
    static void                         PSRAW();

	//Mmi0
	static void							PSUBW();
	static void							PADDH();
	static void							PCGTH();
	static void							PMAXH();
	static void							PSUBB();
	static void							PADDSW();
	static void							PEXTLW();
	static void							PEXTLH();
	static void							PPACH();
	static void							PEXTLB();
	static void							PPACB();
	static void							PEXT5();

	//Mmi1
	static void							PCEQW();
	static void							PMINH();
	static void							PADDUW();
	static void							PEXTUW();
	static void							PEXTUB();
	static void							QFSRV();

	//Mmi2
	static void							PCPYLD();
	static void							PAND();
	static void							PXOR();
    static void                         PROT3W();

	//Mmi3
	static void							PCPYUD();
	static void							POR();
	static void							PNOR();
	static void							PCPYH();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflMmi[64];
	static MIPSReflection::INSTRUCTION	m_cReflMmi0[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi1[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi2[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi3[32];
};

extern CMA_EE g_MAEE;

#endif
