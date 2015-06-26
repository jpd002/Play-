#pragma once

#include "MIPSCoprocessor.h"
#include "MIPSReflection.h"

class CCOP_SCU : public CMIPSCoprocessor
{
public:
	enum REGISTER
	{
		COUNT		= 0x09,
		STATUS		= 0x0C,
		EPC			= 0x0E,
		CPCOND0		= 0x15,
		ERROREPC	= 0x1E,
	};

										CCOP_SCU(MIPS_REGSIZE);
	virtual void						CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
	virtual void						GetInstruction(uint32, char*) override;
	virtual void						GetArguments(uint32, uint32, char*) override;
	virtual uint32						GetEffectiveAddress(uint32, uint32) override;
	virtual MIPS_BRANCH_TYPE			IsBranch(uint32) override;

	static const char*					m_sRegName[];

protected:
	void								SetupReflectionTables();

	static void							ReflOpRtRd(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpCcOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	static uint32						ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION			m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION			m_ReflCop0[32];
	MIPSReflection::INSTRUCTION			m_ReflBc0[4];
	MIPSReflection::INSTRUCTION			m_ReflC0[64];

	MIPSReflection::SUBTABLE			m_ReflGeneralTable;
	MIPSReflection::SUBTABLE			m_ReflCop0Table;
	MIPSReflection::SUBTABLE			m_ReflBc0Table;
	MIPSReflection::SUBTABLE			m_ReflC0Table;


private:
	typedef void (CCOP_SCU::*InstructionFuncConstant)();

	static InstructionFuncConstant		m_pOpGeneral[0x20];
	static InstructionFuncConstant		m_pOpBC0[0x20];
	static InstructionFuncConstant		m_pOpC0[0x40];

	uint8								m_nRT;
	uint8								m_nRD;

	//General
	void								MFC0();
	void								MTC0();
	void								BC0();
	void								C0();

	//BC0
	void								BC0F();
	void								BC0T();
	void								BC0FL();

	//C0
	void								TLBWI();
	void								ERET();
	void								EI();
	void								DI();

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION	m_cReflCop0[32];
	static MIPSReflection::INSTRUCTION	m_cReflBc0[4];
	static MIPSReflection::INSTRUCTION	m_cReflC0[64];
};
