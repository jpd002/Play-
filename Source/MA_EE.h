#ifndef _MA_EE_H_
#define _MA_EE_H_

#include "MA_MIPSIV.h"

class CMA_EE : public CMA_MIPSIV
{
public:
										CMA_EE();
	virtual                             ~CMA_EE();

protected:
	typedef void (CMA_EE::*InstructionFuncConstant)();

	void								SetupReflectionTables();

	static InstructionFuncConstant      m_pOpMmi0[0x20];
	static InstructionFuncConstant      m_pOpMmi1[0x20];
	static InstructionFuncConstant      m_pOpMmi2[0x20];
	static InstructionFuncConstant      m_pOpMmi3[0x20];

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

	void							    PushVector(unsigned int);
	void							    PullVector(unsigned int);
    size_t                              GetLoOffset(unsigned int);
    size_t                              GetHiOffset(unsigned int);

	//General
	void							    LQ();
	void							    SQ();

	//Special
	void							    REEXCPT();

	//RegImm
	void							    MTSAB();
	void							    MTSAH();

	//Special2
	void							    MADD();
	void							    PLZCW();
	void							    MMI0();
	void							    MMI2();
	void							    MFHI1();
	void							    MTHI1();
	void							    MFLO1();
	void							    MTLO1();
	void							    MULT1();
	void							    MULTU1();
	void							    DIV1();
	void							    DIVU1();
    void                                MADD1();
	void							    MMI1();
	void							    MMI3();
	void							    PSLLH();
	void							    PSRLH();
	void							    PSRAH();
    void                                PSLLW();
    void                                PSRLW();
    void                                PSRAW();

	//Mmi0
    void                                PADDW();
	void							    PSUBW();
	void							    PADDH();
	void							    PCGTH();
	void							    PMAXH();
    void                                PADDB();
	void							    PSUBB();
	void							    PADDSW();
	void							    PEXTLW();
    void                                PPACW();
	void							    PEXTLH();
	void							    PPACH();
	void							    PEXTLB();
	void							    PPACB();
	void							    PEXT5();

	//Mmi1
	void							    PCEQW();
	void								PMINW();
	void							    PMINH();
	void							    PADDUW();
	void							    PEXTUW();
	void							    PEXTUB();
	void							    QFSRV();

	//Mmi2
	void								PMFHI();
    void                                PMFLO();
	void								PMULTW();
    void                                PDIVW();
	void							    PCPYLD();
	void							    PAND();
	void							    PXOR();
    void                                PMULTH();
    void                                PEXEW();
    void                                PROT3W();

	//Mmi3
	void							    PCPYUD();
	void							    POR();
	void							    PNOR();
	void							    PCPYH();
	void								PEXCW();

    void                                Generic_MADD(unsigned int);

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflMmi[64];
	static MIPSReflection::INSTRUCTION	m_cReflMmi0[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi1[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi2[32];
	static MIPSReflection::INSTRUCTION	m_cReflMmi3[32];
};

#endif
