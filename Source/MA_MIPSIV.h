#pragma once

#include <functional>
#include "MIPSArchitecture.h"
#include "MIPSReflection.h"

class CMA_MIPSIV : public CMIPSArchitecture
{
public:
										CMA_MIPSIV(MIPS_REGSIZE);
	virtual								~CMA_MIPSIV();
	virtual void						CompileInstruction(uint32, CMipsJitter*, CMIPS*);
	virtual void						GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual void						GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual MIPS_BRANCH_TYPE			IsInstructionBranch(CMIPS*, uint32, uint32);
	virtual uint32						GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);

protected:
	enum
	{
		MAX_GENERAL_OPS = 0x40,
		MAX_SPECIAL_OPS = 0x40,
		MAX_SPECIAL2_OPS = 0x40,
		MAX_REGIMM_OPS = 0x20,
	};

	typedef std::function<void ()> InstructionFunction;

	InstructionFunction					m_pOpGeneral[MAX_GENERAL_OPS];
	InstructionFunction					m_pOpSpecial[MAX_SPECIAL_OPS];
	InstructionFunction					m_pOpSpecial2[MAX_SPECIAL2_OPS];
	InstructionFunction					m_pOpRegImm[MAX_REGIMM_OPS];

	static void							ReflOpTarget(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtRsImm(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtImm(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsRtOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpHintOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpIdOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRdRsRt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRdRtSa(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRdRtRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRd(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRdRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsRt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	static uint32						ReflEaTarget(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);
	static uint32						ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	static void							ReflCOPMnemonic(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);
	static void							ReflCOPOperands(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static MIPS_BRANCH_TYPE				ReflCOPIsBranch(MIPSReflection::INSTRUCTION*, CMIPS*, uint32);
	static uint32						ReflCOPEffeAddr(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION			m_ReflGeneral[MAX_GENERAL_OPS];
	MIPSReflection::INSTRUCTION			m_ReflSpecial[MAX_SPECIAL_OPS];
	MIPSReflection::INSTRUCTION			m_ReflRegImm[MAX_REGIMM_OPS];

	MIPSReflection::SUBTABLE			m_ReflGeneralTable;
	MIPSReflection::SUBTABLE			m_ReflSpecialTable;
	MIPSReflection::SUBTABLE			m_ReflRegImmTable;

	uint8								m_nRS;
	uint8								m_nRT;
	uint8								m_nRD;
	uint8								m_nSA;
	uint16								m_nImmediate;

protected:
	//Instruction compiler templates
	typedef std::function<void (uint8)> TemplateParamedOperationFunctionType;
	typedef std::function<void ()> TemplateOperationFunctionType;

	void Template_Add32(bool);
	void Template_Add64(bool);
	void Template_Sub32(bool);
	void Template_LoadUnsigned32(void*);
	void Template_ShiftCst32(const TemplateParamedOperationFunctionType&);
	void Template_ShiftVar32(const TemplateOperationFunctionType&);
	void Template_Mult32(bool, unsigned int);
	void Template_Div32(bool, unsigned int, unsigned int = 0);
	void Template_MovEqual(bool);
	void Template_SetLessThanImm(bool);
	void Template_SetLessThanReg(bool);
	void Template_BranchEq(bool, bool);
	void Template_BranchGez(bool, bool);
	void Template_BranchLez(bool, bool);

private:
	void							SetupInstructionTables();
	void							SetupReflectionTables();

	void							SPECIAL();
	void							SPECIAL2();
	void							REGIMM();

	//General
	void							J();
	void							JAL();
	void							BEQ();
	void							BNE();
	void							BLEZ();
	void							BGTZ();
	void							ADDI();
	void							ADDIU();
	void							SLTI();
	void							SLTIU();
	void							ANDI();
	void							ORI();
	void							XORI();
	void							LUI();
	void							COP0();
	void							COP1();
	void							COP2();
	void							BEQL();
	void							BNEL();
	void							BLEZL();
	void							BGTZL();
	void							DADDI();
	void							DADDIU();
	void							LDL();
	void							LDR();
	void							LB();
	void							LH();
	void							LWL();
	void							LW();
	void							LBU();
	void							LHU();
	void							LWR();
	void							LWU();
	void							SB();
	void							SH();
	void							SWL();
	void							SW();
	void							SDL();
	void							SDR();
	void							SWR();
	void							CACHE();
	void							LWC1();
	void							PREF();
	void							LDC2();
	void							LD();
	void							SWC1();
	void							SDC2();
	void							SD();

	//Special	
	void							SLL();
	void							SRL();
	void							SRA();
	void							SLLV();
	void							SRLV();
	void							SRAV();
	void							JR();
	void							JALR();
	void							MOVZ();
	void							MOVN();
	void							SYSCALL();
	void							BREAK();
	void							SYNC();
	void							DSLLV();
	void							DSRLV();
	void							DSRAV();
	void							MFHI();
	void							MTHI();
	void							MFLO();
	void							MTLO();
	void							MULT();
	void							MULTU();
	void							DIV();
	void							DIVU();
	void							ADD();
	void							ADDU();
	void							SUB();
	void							SUBU();
	void							AND();
	void							OR();
	void							XOR();
	void							NOR();
	void							SLT();
	void							SLTU();
	void							DADD();
	void							DADDU();
	void							DSUBU();
	void							DSLL();
	void							DSRL();
	void							DSRA();
	void							DSLL32();
	void							DSRL32();
	void							DSRA32();

	//Special2

	//RegImm
	void							BLTZ();
	void							BGEZ();
	void							BLTZL();
	void							BGEZL();
	void							BLTZAL();
	void							BGEZAL();
	void							BLTZALL();
	void							BGEZALL();

	//Opcode tables
	typedef void (CMA_MIPSIV::*InstructionFuncConstant)();

	static InstructionFuncConstant		m_cOpGeneral[MAX_GENERAL_OPS];
	static InstructionFuncConstant		m_cOpSpecial[MAX_SPECIAL_OPS];
	static InstructionFuncConstant		m_cOpRegImm[MAX_REGIMM_OPS];

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[MAX_GENERAL_OPS];
	static MIPSReflection::INSTRUCTION	m_cReflSpecial[MAX_SPECIAL_OPS];
	static MIPSReflection::INSTRUCTION	m_cReflRegImm[MAX_REGIMM_OPS];
};
