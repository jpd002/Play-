#pragma once

#include "../MIPSArchitecture.h"
#include "../MIPSReflection.h"
#include "VUShared.h"

#undef MAX
#undef ABS

class CMA_VU : public CMIPSArchitecture
{
public:
											CMA_VU();
	virtual									~CMA_VU();
	virtual void							CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
	virtual void							GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int) override;
	virtual void							GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int) override;
	virtual MIPS_BRANCH_TYPE				IsInstructionBranch(CMIPS*, uint32, uint32) override;
	virtual uint32							GetInstructionEffectiveAddress(CMIPS*, uint32, uint32) override;
	VUShared::OPERANDSET					GetAffectedOperands(CMIPS*, uint32, uint32);

	void									SetRelativePipeTime(uint32);

private:
	void									SetupReflectionTables();

	class CUpper : public CMIPSInstructionFactory
	{
	public:
											CUpper();

		void								SetupReflectionTables();
		void								CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
		void								GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
		void								GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
		VUShared::OPERANDSET				GetAffectedOperands(CMIPS*, uint32, uint32);
		MIPS_BRANCH_TYPE					IsInstructionBranch(CMIPS*, uint32, uint32);
		uint32								GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);

		void								SetRelativePipeTime(uint32);

	private:
		typedef void (CUpper::*InstructionFuncConstant)();

		static InstructionFuncConstant		m_pOpVector[0x40];
		static InstructionFuncConstant		m_pOpVector0[0x20];
		static InstructionFuncConstant		m_pOpVector1[0x20];
		static InstructionFuncConstant		m_pOpVector2[0x20];
		static InstructionFuncConstant		m_pOpVector3[0x20];

		uint8								m_nFT;
		uint8								m_nFS;
		uint8								m_nFD;
		uint8								m_nBc;
		uint8								m_nDest;
		uint32								m_relativePipeTime;

		static void							ReflOpFtFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

		void								LOI(uint32);

		//Vector
		void								ADDbc();
		void								SUBbc();
		void								MADDbc();
		void								MSUBbc();
		void								MAXbc();
		void								MINIbc();
		void								MULbc();
		void								MULq();
		void								MAXi();
		void								MULi();
		void								MINIi();
		void								ADDq();
		void								MADDq();
		void								ADDi();
		void								MADDi();
		void								SUBq();
		void								SUBi();
		void								MSUBi();
		void								ADD();
		void								MADD();
		void								MUL();
		void								MAX();
		void								SUB();
		void								MSUB();
		void								OPMSUB();
		void								MINI();
		void								VECTOR0();
		void								VECTOR1();
		void								VECTOR2();
		void								VECTOR3();

		//Vector X Common
		void								ADDAbc();
		void								SUBAbc();
		void								MADDAbc();
		void								MSUBAbc();
		void								MULAbc();

		//Vector 0
		void								ITOF0();
		void								FTOI0();
		void								MULAq();
		void								ADDA();
		void								SUBA();

		//Vector 1
		void								ITOF4();
		void								FTOI4();
		void								ABS();
		void								MADDA();
		void								MSUBA();

		//Vector 2
		void								ITOF12();
		void								FTOI12();
		void								MULAi();
		void								ADDAi();
		void								SUBAi();
		void								MULA();
		void								OPMULA();

		//Vector 3
		void								ITOF15();
		void								FTOI15();
		void								CLIP();
		void								MADDAi();
		void								MSUBAi();
		void								NOP();

		MIPSReflection::INSTRUCTION			m_ReflV[64];
		MIPSReflection::INSTRUCTION			m_ReflVX0[32];
		MIPSReflection::INSTRUCTION			m_ReflVX1[32];
		MIPSReflection::INSTRUCTION			m_ReflVX2[32];
		MIPSReflection::INSTRUCTION			m_ReflVX3[32];

		VUShared::VUINSTRUCTION				m_VuReflV[64];
		VUShared::VUINSTRUCTION				m_VuReflVX0[32];
		VUShared::VUINSTRUCTION				m_VuReflVX1[32];
		VUShared::VUINSTRUCTION				m_VuReflVX2[32];
		VUShared::VUINSTRUCTION				m_VuReflVX3[32];

		MIPSReflection::SUBTABLE			m_ReflVTable;
		MIPSReflection::SUBTABLE			m_ReflVX0Table;
		MIPSReflection::SUBTABLE			m_ReflVX1Table;
		MIPSReflection::SUBTABLE			m_ReflVX2Table;
		MIPSReflection::SUBTABLE			m_ReflVX3Table;

		VUShared::VUSUBTABLE				m_VuReflVTable;
		VUShared::VUSUBTABLE				m_VuReflVX0Table;
		VUShared::VUSUBTABLE				m_VuReflVX1Table;
		VUShared::VUSUBTABLE				m_VuReflVX2Table;
		VUShared::VUSUBTABLE				m_VuReflVX3Table;

		static MIPSReflection::INSTRUCTION	m_cReflV[64];
		static MIPSReflection::INSTRUCTION	m_cReflVX0[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX1[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX2[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX3[32];

		static VUShared::VUINSTRUCTION		m_cVuReflV[64];
		static VUShared::VUINSTRUCTION		m_cVuReflVX0[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX1[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX2[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX3[32];
	};

	class CLower : public CMIPSInstructionFactory
	{
	public:
											CLower();
		virtual								~CLower();

		void								SetupReflectionTables();
		void								CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
		void								GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
		void								GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
		MIPS_BRANCH_TYPE					IsInstructionBranch(CMIPS*, uint32, uint32);
		uint32								GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);
		VUShared::OPERANDSET				GetAffectedOperands(CMIPS*, uint32, uint32);

		void								SetRelativePipeTime(uint32);

	private:
		enum
		{
			OPCODE_NOP = 0x8000033C
		};

		typedef void (CLower::*InstructionFuncConstant)();

		static InstructionFuncConstant		m_pOpGeneral[0x80];
		static InstructionFuncConstant		m_pOpLower[0x40];
		static InstructionFuncConstant		m_pOpVector0[0x20];
		static InstructionFuncConstant		m_pOpVector1[0x20];
		static InstructionFuncConstant		m_pOpVector2[0x20];
		static InstructionFuncConstant		m_pOpVector3[0x20];

		uint8								m_nIT;
		uint8								m_nIS;
		uint8								m_nID;
		uint8								m_nFSF;
		uint8								m_nFTF;
		uint8								m_nDest;
		uint8								m_nImm5;
		uint16								m_nImm11;
		uint16								m_nImm12;
		uint16								m_nImm15;
		uint16								m_nImm15S;
		uint32								m_nImm24;
		uint32								m_relativePipeTime;

		void								SetBranchAddress(bool, int32);
		static bool							IsLOI(CMIPS*, uint32);

		static void							ReflOpIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpIsOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpImm12(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItImm12(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIsOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIsImm15(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItOfsIsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpImm24(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpVi1Imm24(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstIsDec(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFsDstOfsIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstOfsIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstFsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpPFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpPFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtP(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

		static uint32						ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);
		static uint32						ReflEaIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

		static void							ReflOpAffRdIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffRdItFs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffRdItIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffRdP(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrFtRdFs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrFtRdIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrFtRdP(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrFtIsRdIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrIdRdItIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrIt(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrItRdFs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrItRdIs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrItRdItFs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrPRdFs(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);
		static void							ReflOpAffWrVi1(VUShared::VUINSTRUCTION*, CMIPS*, uint32, uint32, VUShared::OPERANDSET&);

		void								BuildStatusInIT();

		//General
		void								LQ();
		void								SQ();
		void								ILW();
		void								ISW();
		void								IADDIU();
		void								ISUBIU();
		void								FCEQ();
		void								FCSET();
		void								FCAND();
		void								FCOR();
		void								FSSET();
		void								FSAND();
		void								FSOR();
		void								FMEQ();
		void								FMAND();
		void								FMOR();
		void								FCGET();
		void								B();
		void								BAL();
		void								JR();
		void								JALR();
		void								IBEQ();
		void								IBNE();
		void								IBLTZ();
		void								IBGTZ();
		void								IBLEZ();
		void								IBGEZ();
		void								LOWEROP();

		//LowerOp
		void								IADD();
		void								ISUB();
		void								IADDI();
		void								IAND();
		void								IOR();
		void								VECTOR0();
		void								VECTOR1();
		void								VECTOR2();
		void								VECTOR3();

		//Vector0
		void								MOVE();
		void								LQI();
		void								DIV();
		void								MTIR();
		void								RNEXT();
		void								MFP();
		void								XTOP();
		void								XGKICK();
		void								ESQRT();
		void								ESIN();

		//Vector1
		void								MR32();
		void								SQI();
		void								SQRT();
		void								MFIR();
		void								RGET();
		void								XITOP();
		void								ERSQRT();

		//Vector2
		void								LQD();
		void								RSQRT();
		void								ILWR();
		void								RINIT();
		void								ELENG();
		void								ESUM();
		void								ERCPR();

		//Vector3
		void								SQD();
		void								WAITQ();
		void								ISWR();
		void								ERLENG();
		void								WAITP();

		MIPSReflection::INSTRUCTION			m_ReflGeneral[128];
		MIPSReflection::INSTRUCTION			m_ReflV[64];
		MIPSReflection::INSTRUCTION			m_ReflVX0[32];
		MIPSReflection::INSTRUCTION			m_ReflVX1[32];
		MIPSReflection::INSTRUCTION			m_ReflVX2[32];
		MIPSReflection::INSTRUCTION			m_ReflVX3[32];

		VUShared::VUINSTRUCTION				m_VuReflGeneral[128];
		VUShared::VUINSTRUCTION				m_VuReflV[64];
		VUShared::VUINSTRUCTION				m_VuReflVX0[32];
		VUShared::VUINSTRUCTION				m_VuReflVX1[32];
		VUShared::VUINSTRUCTION				m_VuReflVX2[32];
		VUShared::VUINSTRUCTION				m_VuReflVX3[32];

		MIPSReflection::SUBTABLE			m_ReflGeneralTable;
		MIPSReflection::SUBTABLE			m_ReflVTable;
		MIPSReflection::SUBTABLE			m_ReflVX0Table;
		MIPSReflection::SUBTABLE			m_ReflVX1Table;
		MIPSReflection::SUBTABLE			m_ReflVX2Table;
		MIPSReflection::SUBTABLE			m_ReflVX3Table;

		VUShared::VUSUBTABLE				m_VuReflGeneralTable;
		VUShared::VUSUBTABLE				m_VuReflVTable;
		VUShared::VUSUBTABLE				m_VuReflVX0Table;
		VUShared::VUSUBTABLE				m_VuReflVX1Table;
		VUShared::VUSUBTABLE				m_VuReflVX2Table;
		VUShared::VUSUBTABLE				m_VuReflVX3Table;

		static MIPSReflection::INSTRUCTION	m_cReflGeneral[128];
		static MIPSReflection::INSTRUCTION	m_cReflV[64];
		static MIPSReflection::INSTRUCTION	m_cReflVX0[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX1[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX2[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX3[32];

		static VUShared::VUINSTRUCTION		m_cVuReflGeneral[128];
		static VUShared::VUINSTRUCTION		m_cVuReflV[64];
		static VUShared::VUINSTRUCTION		m_cVuReflVX0[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX1[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX2[32];
		static VUShared::VUINSTRUCTION		m_cVuReflVX3[32];
	};

	CUpper									m_Upper;
	CLower									m_Lower;
};
