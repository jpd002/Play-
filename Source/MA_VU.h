#ifndef _MA_VU_H_
#define _MA_VU_H_

#include "MIPSArchitecture.h"
#include "MIPSReflection.h"

#undef MAX
#undef ABS

class CMA_VU : public CMIPSArchitecture
{
public:
											CMA_VU(bool);
    virtual                                 ~CMA_VU();
	virtual void							CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
	virtual void							GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual void							GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
	virtual bool							IsInstructionBranch(CMIPS*, uint32, uint32);
	virtual uint32							GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);

private:
	void									SetupReflectionTables();

	class CUpper
	{
	public:
		void								SetupReflectionTables();
		void								CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
		void								GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
		void								GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
		bool								IsInstructionBranch(CMIPS*, uint32, uint32);
		uint32								GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);
	
	private:
		static void							(*m_pOpVector[0x40])();
		static void							(*m_pOpVector0[0x20])();
		static void							(*m_pOpVector1[0x20])();
		static void							(*m_pOpVector2[0x20])();
		static void							(*m_pOpVector3[0x20])();

		static uint8						m_nFT;
		static uint8						m_nFS;
		static uint8						m_nFD;
		static uint8						m_nBc;
		static uint8						m_nDest;

		static void							ReflOpFtFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

		static void							LOI(uint32);

		//Vector
		static void							ADDbc();
		static void							SUBbc();
		static void							MADDbc();
        static void                         MSUBbc();
		static void							MAXbc();
		static void							MINIbc();
		static void							MULbc();
		static void							MULq();
		static void							MULi();
		static void							MINIi();
		static void							ADDq();
        static void                         MADDq();
		static void							ADDi();
		static void							SUBi();
		static void							MSUBi();
		static void							ADD();
		static void							MADD();
		static void							MUL();
		static void							MAX();
		static void							SUB();
		static void							OPMSUB();
        static void                         MINI();
		static void							VECTOR0();
		static void							VECTOR1();
		static void							VECTOR2();
		static void							VECTOR3();

		//Vector X Common
		static void							ADDAbc();
		static void							MADDAbc();
        static void                         MSUBAbc();
		static void							MULAbc();

		//Vector 0
		static void							ITOF0();
		static void							FTOI0();
        static void                         ADDA();

		//Vector 1
        static void                         ITOF4();
		static void							FTOI4();
		static void							ABS();
		static void							MADDA();

		//Vector 2
        static void                         ITOF12();
        static void                         FTOI12();
		static void							MULAi();
		static void							MULA();
		static void							OPMULA();

		//Vector 3
		static void							CLIP();
		static void							MADDAi();
		static void							MSUBAi();
		static void							NOP();

		MIPSReflection::INSTRUCTION			m_ReflV[64];
		MIPSReflection::INSTRUCTION			m_ReflVX0[32];
		MIPSReflection::INSTRUCTION			m_ReflVX1[32];
		MIPSReflection::INSTRUCTION			m_ReflVX2[32];
		MIPSReflection::INSTRUCTION			m_ReflVX3[32];

		MIPSReflection::SUBTABLE			m_ReflVTable;
		MIPSReflection::SUBTABLE			m_ReflVX0Table;
		MIPSReflection::SUBTABLE			m_ReflVX1Table;
		MIPSReflection::SUBTABLE			m_ReflVX2Table;
		MIPSReflection::SUBTABLE			m_ReflVX3Table;

		static MIPSReflection::INSTRUCTION	m_cReflV[64];
		static MIPSReflection::INSTRUCTION	m_cReflVX0[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX1[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX2[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX3[32];
	};

	class CLower
	{
	public:
                                            CLower(bool);
        virtual                             ~CLower();

		void								SetupReflectionTables();
		void								CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
		void								GetInstructionMnemonic(CMIPS*, uint32, uint32, char*, unsigned int);
		void								GetInstructionOperands(CMIPS*, uint32, uint32, char*, unsigned int);
		bool								IsInstructionBranch(CMIPS*, uint32, uint32);
		uint32								GetInstructionEffectiveAddress(CMIPS*, uint32, uint32);

        static void                         GetQuadWord(uint32, CMIPS*, uint32, uint32);
        static void                         SetQuadWord(uint32, CMIPS*, uint32, uint32);

	private:
		static void							(*m_pOpGeneral[0x80])();
		static void							(*m_pOpLower[0x40])();
		static void							(*m_pOpVector0[0x20])();
		static void							(*m_pOpVector1[0x20])();
		static void							(*m_pOpVector2[0x20])();
		static void							(*m_pOpVector3[0x20])();

		static uint8						m_nIT;
		static uint8						m_nIS;
		static uint8						m_nID;
		static uint8						m_nFSF;
		static uint8						m_nFTF;
		static uint8						m_nDest;
		static uint8						m_nImm5;
		static uint16						m_nImm11;
        static uint16                       m_nImm12;
		static uint16						m_nImm15;
		static uint16						m_nImm15S;
		static uint32						m_nImm24;

		static uint32						GetDestOffset(uint8);
		static void							SetBranchAddress(bool, int32);
		static void							PushIntegerRegister(unsigned int);
        static void                         ComputeMemAccessAddr(unsigned int, uint32, uint32);

		static void							ReflOpIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpIsOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
        static void                         ReflOpItImm12(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
        static void							ReflOpItIsOfs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIsImm5(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIsImm15(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpIdIsIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItIsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpItOfsIsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpImm24(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpVi1Imm24(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstIsInc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstIsDec(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFsDstItInc(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFsDstOfsIt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstOfsIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtDstFsDst(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpPFs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpPFsf(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
		static void							ReflOpFtP(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

		static uint32						ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);
		static uint32						ReflEaIs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

		//General
		static void							LQ();
		static void							SQ();
		static void							ILW();
		static void							ISW();
		static void							IADDIU();
		static void							ISUBIU();
		static void							FCSET();
		static void							FCAND();
        static void                         FCOR();
        static void                         FSAND();
		static void							FMAND();
        static void                         FCGET();
		static void							B();
        static void                         BAL();
        static void                         JR();
		static void							JALR();
		static void							IBEQ();
		static void							IBNE();
		static void							IBLTZ();
		static void							IBGTZ();
		static void							IBLEZ();
		static void							IBGEZ();
		static void							LOWEROP();

		//LowerOp
		static void							IADD();
		static void							ISUB();
		static void							IADDI();
		static void							IAND();
		static void							IOR();
		static void							VECTOR0();
		static void							VECTOR1();
		static void							VECTOR2();
		static void							VECTOR3();

		//Vector0
		static void							MOVE();
		static void							LQI();
		static void							DIV();
		static void							MTIR();
		static void							MFP();
		static void							XTOP();
		static void							XGKICK();
        static void                         ESIN();

		//Vector1
		static void							MR32();
		static void							SQI();
		static void							MFIR();
		static void							RGET();
        static void                         XITOP();

		//Vector2
        static void                         RSQRT();
		static void							ILWR();
		static void							RINIT();
		static void							ERCPR();

		//Vector3
		static void							WAITQ();
		static void							ERLENG();
        static void                         WAITP();

		MIPSReflection::INSTRUCTION			m_ReflGeneral[128];
		MIPSReflection::INSTRUCTION			m_ReflV[64];
		MIPSReflection::INSTRUCTION			m_ReflVX0[32];
		MIPSReflection::INSTRUCTION			m_ReflVX1[32];
		MIPSReflection::INSTRUCTION			m_ReflVX2[32];
		MIPSReflection::INSTRUCTION			m_ReflVX3[32];

		MIPSReflection::SUBTABLE			m_ReflGeneralTable;
		MIPSReflection::SUBTABLE			m_ReflVTable;
		MIPSReflection::SUBTABLE			m_ReflVX0Table;
		MIPSReflection::SUBTABLE			m_ReflVX1Table;
		MIPSReflection::SUBTABLE			m_ReflVX2Table;
		MIPSReflection::SUBTABLE			m_ReflVX3Table;

		static MIPSReflection::INSTRUCTION	m_cReflGeneral[128];
		static MIPSReflection::INSTRUCTION	m_cReflV[64];
		static MIPSReflection::INSTRUCTION	m_cReflVX0[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX1[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX2[32];
		static MIPSReflection::INSTRUCTION	m_cReflVX3[32];

        bool                                m_maskDataAddress;
	};

	CUpper									m_Upper;
	CLower									m_Lower;

    static CCodeGen*                        m_codeGen;
    static CMIPS*                           m_pCtx;
    static uint32                           m_nOpcode;
    static uint32                           m_nAddress;
};

#endif
