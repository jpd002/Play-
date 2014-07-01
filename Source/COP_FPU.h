#pragma once

#include "MIPSCoprocessor.h"
#include "MIPSReflection.h"

class CCOP_FPU : public CMIPSCoprocessor
{
public:
										CCOP_FPU(MIPS_REGSIZE);
	virtual void						CompileInstruction(uint32, CMipsJitter*, CMIPS*);
	virtual void						GetInstruction(uint32, char*);
	virtual void						GetArguments(uint32, uint32, char*);
	virtual uint32						GetEffectiveAddress(uint32, uint32);
	virtual MIPS_BRANCH_TYPE			IsBranch(uint32);

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
	typedef void (CCOP_FPU::*InstructionFuncConstant)();

	uint8								m_nFT;
	uint8								m_nFS;
	uint8								m_nFD;

	static uint32						m_nCCMask[8];

	void								SetCCBit(bool, uint32);
	void								PushCCBit(uint32);

	static InstructionFuncConstant		m_pOpGeneral[0x20];
	static InstructionFuncConstant		m_pOpSingle[0x40];
	static InstructionFuncConstant		m_pOpWord[0x40];

	//General
	void								MFC1();
	void								CFC1();
	void								MTC1();
	void								CTC1();
	void								BC1();
	void								S();
	void								W();

	//Branch
	void								BC1F();
	void								BC1T();
	void								BC1FL();
	void								BC1TL();

	//Single
	void								ADD_S();
	void								SUB_S();
	void								MUL_S();
	void								DIV_S();
	void								SQRT_S();
	void								ABS_S();
	void								MOV_S();
	void								NEG_S();
	void								TRUNC_W_S();
	void								RSQRT_S();
	void								ADDA_S();
	void								SUBA_S();
	void								MULA_S();
	void								MADD_S();
	void								MSUB_S();
	void								MADDA_S();
	void								MSUBA_S();
	void								CVT_W_S();
	void								MAX_S();
	void								MIN_S();
	void								C_EQ_S();
	void								C_LT_S();
	void								C_LE_S();

	//Word
	void								CVT_S_W();

	//Misc
	void								LWC1();
	void								SWC1();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION	m_cReflCop1[32];
	static MIPSReflection::INSTRUCTION	m_cReflBc1[4];
	static MIPSReflection::INSTRUCTION	m_cReflS[64];
	static MIPSReflection::INSTRUCTION	m_cReflW[64];
};
