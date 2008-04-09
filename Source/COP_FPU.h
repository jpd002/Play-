#ifndef _COP_FPU_H_
#define _COP_FPU_H_

#include "MIPSCoprocessor.h"
#include "MIPSReflection.h"

class CCOP_FPU : public CMIPSCoprocessor
{
public:
										CCOP_FPU(MIPS_REGSIZE);
	virtual void						CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
	virtual void						GetInstruction(uint32, char*);
	virtual void						GetArguments(uint32, uint32, char*);
	virtual uint32						GetEffectiveAddress(uint32, uint32);
	virtual bool						IsBranch(uint32);

protected:
	void								SetupReflectionTables();

	static void							ReflOpRtFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtFcs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFdFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFdFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpCcFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFdFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFtOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpCcOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	static uint32						ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION			m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION			m_ReflCop1[32];
	MIPSReflection::INSTRUCTION			m_ReflBc1[4];
	MIPSReflection::INSTRUCTION			m_ReflS[64];
	MIPSReflection::INSTRUCTION			m_ReflW[64];

	MIPSReflection::SUBTABLE			m_ReflGeneralTable;
	MIPSReflection::SUBTABLE			m_ReflCop1Table;
	MIPSReflection::SUBTABLE			m_ReflBc1Table;
	MIPSReflection::SUBTABLE			m_ReflSTable;
	MIPSReflection::SUBTABLE			m_ReflWTable;

private:
	static uint8						m_nFT;
	static uint8						m_nFS;
	static uint8						m_nFD;

	static uint32						m_nCCMask[8];

	static void							SetCCBit(bool, uint32);
	static void							TestCCBit(uint32);

	static void							(*m_pOpGeneral[0x20])();
	static void							(*m_pOpSingle[0x40])();
	static void							(*m_pOpWord[0x40])();

	//General
	static void							MFC1();
	static void							MTC1();
	static void							CTC1();
	static void							BC1();
	static void							S();
	static void							W();

	//Branch
	static void							BC1F();
	static void							BC1T();
	static void							BC1FL();
	static void							BC1TL();

	//Single
	static void							ADD_S();
	static void							SUB_S();
	static void							MUL_S();
	static void							DIV_S();
	static void							SQRT_S();
	static void							ABS_S();
	static void							MOV_S();
	static void							NEG_S();
	static void							ADDA_S();
	static void							MULA_S();
	static void							MADD_S();
	static void							MSUB_S();
	static void							CVT_W_S();
	static void							C_EQ_S();
	static void							C_LT_S();
	static void							C_LE_S();

	//Word
	static void							CVT_S_W();

	//Misc
	static void							LWC1();
	static void							SWC1();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION	m_cReflCop1[32];
	static MIPSReflection::INSTRUCTION	m_cReflBc1[4];
	static MIPSReflection::INSTRUCTION	m_cReflS[64];
	static MIPSReflection::INSTRUCTION	m_cReflW[64];
};

extern CCOP_FPU g_COPFPU;

#endif
