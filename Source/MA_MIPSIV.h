#ifndef _MA_MIPSIV_H_
#define _MA_MIPSIV_H_

#include <functional>
#include "MIPSArchitecture.h"
#include "MIPSReflection.h"

class CMA_MIPSIV : public CMIPSArchitecture
{
public:
										CMA_MIPSIV(MIPS_REGSIZE);
	virtual void						CompileInstruction(uint32, CCacheBlock*, CMIPS*, bool);
	virtual void						GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual void						GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual bool						IsInstructionBranch(CMIPS*, uint32, uint32);
	virtual uint32						GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);

protected:
	void								SetupReflectionTables();

	static void							(*m_pOpGeneral[0x40])();
	static void							(*m_pOpSpecial[0x40])();
	static void							(*m_pOpSpecial2[0x40])();
	static void							(*m_pOpRegImm[0x20])();

	static void							ReflOpTarget(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtRsImm(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtImm(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsRtOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRsOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
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
	static bool							ReflCOPIsBranch(MIPSReflection::INSTRUCTION*, CMIPS*, uint32);
	static uint32						ReflCOPEffeAddr(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION			m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION			m_ReflSpecial[64];
	MIPSReflection::INSTRUCTION			m_ReflRegImm[32];

	MIPSReflection::SUBTABLE			m_ReflGeneralTable;
	MIPSReflection::SUBTABLE			m_ReflSpecialTable;
	MIPSReflection::SUBTABLE			m_ReflRegImmTable;

	static uint8						m_nRS;
	static uint8						m_nRT;
	static uint8						m_nRD;
	static uint8						m_nSA;
	static uint16						m_nImmediate;

protected:
    //Instruction compiler templates

    struct Template_LoadUnsigned32
    {
        void operator()(void*);
    };

    struct Template_ShiftCst32
    {
        typedef std::tr1::function<void (uint8)> OperationFunctionType;
        void operator()(const OperationFunctionType&) const;
    };

    struct Template_ShiftVar32
    {
        typedef std::tr1::function<void ()> OperationFunctionType;
        void operator()(const OperationFunctionType&) const;
    };

    struct Template_Mult32
    {
        typedef std::tr1::function<void ()> OperationFunctionType;
        void operator()(const OperationFunctionType&, unsigned int) const;
    };

    struct Template_Div32
    {
        typedef std::tr1::function<void ()> OperationFunctionType;
        void operator()(const OperationFunctionType&, unsigned int) const;
    };

    struct Template_MovEqual
    {
        void operator()(bool) const;
    };

    struct Template_BranchGez
    {
        void operator()(bool, bool) const;
    };

    struct Template_BranchLez
    {
        void operator()(bool, bool) const;
    };

private:
	static void							SPECIAL();
	static void							SPECIAL2();
	static void							REGIMM();

	//General
	static void							J();
	static void							JAL();
	static void							BEQ();
	static void							BNE();
	static void							BLEZ();
	static void							BGTZ();
	static void							ADDI();
	static void							ADDIU();
	static void							SLTI();
	static void							SLTIU();
	static void							ANDI();
	static void							ORI();
	static void							XORI();
	static void							LUI();
	static void							COP0();
	static void							COP1();
	static void							COP2();
	static void							BEQL();
	static void							BNEL();
	static void							BLEZL();
	static void							BGTZL();
	static void							DADDIU();
	static void							LDL();
	static void							LDR();
	static void							LB();
	static void							LH();
	static void							LWL();
	static void							LW();
	static void							LBU();
	static void							LHU();
	static void							LWR();
	static void							LWU();
	static void							SB();
	static void							SH();
	static void							SWL();
	static void							SW();
	static void							SDL();
	static void							SDR();
	static void							SWR();
	static void							CACHE();
	static void							LWC1();
	static void							LDC2();
	static void							LD();
	static void							SWC1();
	static void							SDC2();
	static void							SD();

	//Special	
	static void							SLL();
	static void							SRL();
	static void							SRA();
	static void							SLLV();
	static void							SRLV();
	static void							SRAV();
	static void							JR();
	static void							JALR();
	static void							MOVZ();
	static void							MOVN();
	static void							SYSCALL();
    static void                         BREAK();
	static void							SYNC();
	static void							DSLLV();
	static void							DSRLV();
	static void							MFHI();
	static void							MTHI();
	static void							MFLO();
	static void							MTLO();
	static void							MULT();
	static void							MULTU();
	static void							DIV();
	static void							DIVU();
	static void							ADD();
	static void							ADDU();
	static void							SUBU();
	static void							AND();
	static void							OR();
	static void							XOR();
	static void							NOR();
	static void							SLT();
	static void							SLTU();
	static void							DADDU();
	static void							DSUBU();
	static void							DSLL();
	static void							DSRL();
	static void							DSRA();
	static void							DSLL32();
	static void							DSRL32();
	static void							DSRA32();

	//Special2

	//RegImm
	static void							BLTZ();
	static void							BGEZ();
	static void							BLTZL();
	static void							BGEZL();

    //Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION	m_cReflSpecial[64];
	static MIPSReflection::INSTRUCTION	m_cReflRegImm[32];
};

extern CMA_MIPSIV g_MAMIPSIV;

#endif
