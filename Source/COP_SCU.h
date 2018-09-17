#pragma once

#include "MIPSCoprocessor.h"
#include "MIPSReflection.h"
#include "Convertible.h"

class CCOP_SCU : public CMIPSCoprocessor
{
public:
	enum REGISTER
	{
		INDEX = 0x00,
		ENTRYLO0 = 0x02,
		ENTRYLO1 = 0x03,
		PAGEMASK = 0x05,
		BADVADDR = 0x08,
		COUNT = 0x09,
		ENTRYHI = 0x0A,
		STATUS = 0x0C,
		CAUSE = 0x0D,
		EPC = 0x0E,
		CPCOND0 = 0x15,
		ERROREPC = 0x1E,
	};

	struct PCCR : public convertible<uint32>
	{
		unsigned int reserved0 : 1;
		unsigned int exl0 : 1;
		unsigned int k0 : 1;
		unsigned int s0 : 1;
		unsigned int u0 : 1;
		unsigned int event0 : 5;
		unsigned int reserved1 : 1;
		unsigned int exl1 : 1;
		unsigned int k1 : 1;
		unsigned int s1 : 1;
		unsigned int u1 : 1;
		unsigned int event1 : 5;
		unsigned int reserved2 : 11;
		unsigned int cte : 1;
	};
	static_assert(sizeof(PCCR) == 4, "PCCR must be 4 bytes long.");

	CCOP_SCU(MIPS_REGSIZE);
	virtual void CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
	virtual void GetInstruction(uint32, char*) override;
	virtual void GetArguments(uint32, uint32, char*) override;
	virtual uint32 GetEffectiveAddress(uint32, uint32) override;
	virtual MIPS_BRANCH_TYPE IsBranch(uint32) override;

	static const char* m_sRegName[];

protected:
	void SetupReflectionTables();

	static void ReflOpRt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpRtPcr(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpRtRd(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpCcOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	static uint32 ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION m_ReflMfc0[32];
	MIPSReflection::INSTRUCTION m_ReflMtc0[32];
	MIPSReflection::INSTRUCTION m_ReflCop0[32];
	MIPSReflection::INSTRUCTION m_ReflBc0[4];
	MIPSReflection::INSTRUCTION m_ReflC0[64];
	MIPSReflection::INSTRUCTION m_ReflMfPerf[2];
	MIPSReflection::INSTRUCTION m_ReflMtPerf[2];

	MIPSReflection::SUBTABLE m_ReflGeneralTable;
	MIPSReflection::SUBTABLE m_ReflMfc0Table;
	MIPSReflection::SUBTABLE m_ReflMtc0Table;
	MIPSReflection::SUBTABLE m_ReflCop0Table;
	MIPSReflection::SUBTABLE m_ReflBc0Table;
	MIPSReflection::SUBTABLE m_ReflC0Table;
	MIPSReflection::SUBTABLE m_ReflMfPerfTable;
	MIPSReflection::SUBTABLE m_ReflMtPerfTable;

private:
	typedef void (CCOP_SCU::*InstructionFuncConstant)();

	static InstructionFuncConstant m_pOpGeneral[0x20];
	static InstructionFuncConstant m_pOpMtc0[0x20];
	static InstructionFuncConstant m_pOpBC0[0x20];
	static InstructionFuncConstant m_pOpC0[0x40];

	uint8 m_nRT;
	uint8 m_nRD;

	//General
	void MFC0();
	void MTC0();
	void BC0();
	void C0();

	//BC0
	void BC0F();
	void BC0T();
	void BC0FL();

	//C0
	void TLBWI();
	void ERET();
	void EI();
	void DI();

	//Reflection tables
	static MIPSReflection::INSTRUCTION m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION m_cReflCop0[32];
	static MIPSReflection::INSTRUCTION m_cReflMfc0[32];
	static MIPSReflection::INSTRUCTION m_cReflMtc0[32];
	static MIPSReflection::INSTRUCTION m_cReflBc0[4];
	static MIPSReflection::INSTRUCTION m_cReflC0[64];
	static MIPSReflection::INSTRUCTION m_cReflMfPerf[2];
	static MIPSReflection::INSTRUCTION m_cReflMtPerf[2];
};
