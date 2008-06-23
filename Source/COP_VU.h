#ifndef _COP_VU_H_
#define _COP_VU_H_

#include "MIPSCoprocessor.h"
#include "MIPSReflection.h"

class CCOP_VU : public CMIPSCoprocessor
{
public:
										CCOP_VU(MIPS_REGSIZE);

	virtual void						CompileInstruction(uint32, CCodeGen*, CMIPS*, bool);
	virtual void						GetInstruction(uint32, char*);
	virtual void						GetArguments(uint32, uint32, char*);
	virtual uint32						GetEffectiveAddress(uint32, uint32);
	virtual bool						IsBranch(uint32);

protected:
	void								SetupReflectionTables();

	static void							ReflMnemI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);

	static void							ReflOpRtFd(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpRtId(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpAccFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void							ReflOpFtOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	MIPSReflection::INSTRUCTION			m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION			m_ReflCop2[32];
	MIPSReflection::INSTRUCTION			m_ReflV[64];
	MIPSReflection::INSTRUCTION			m_ReflVX0[32];
	MIPSReflection::INSTRUCTION			m_ReflVX1[32];
	MIPSReflection::INSTRUCTION			m_ReflVX2[32];
	MIPSReflection::INSTRUCTION			m_ReflVX3[32];

	MIPSReflection::SUBTABLE			m_ReflGeneralTable;
	MIPSReflection::SUBTABLE			m_ReflCop2Table;
	MIPSReflection::SUBTABLE			m_ReflVTable;
	MIPSReflection::SUBTABLE			m_ReflVX0Table;
	MIPSReflection::SUBTABLE			m_ReflVX1Table;
	MIPSReflection::SUBTABLE			m_ReflVX2Table;
	MIPSReflection::SUBTABLE			m_ReflVX3Table;

	static void							(*m_pOpCop2[0x20])();
	static void							(*m_pOpVector[0x40])();
	static void							(*m_pOpVx0[0x20])();
	static void							(*m_pOpVx1[0x20])();
	static void							(*m_pOpVx2[0x20])();
	static void							(*m_pOpVx3[0x20])();

private:
	//General
	static void							LQC2();
	static void							SQC2();

	//COP2
	static void							QMFC2();
	static void							CFC2();
	static void							QMTC2();
	static void							CTC2();
	static void							V();

	//Vector
	static void							VADDbc();
	static void							VSUBbc();
	static void							VMADDbc();
    static void                         VMSUBbc();
	static void							VMAXbc();
	static void							VMINIbc();
	static void							VMULbc();
	static void							VMULq();
	static void							VADDq();
	static void							VADD();
	static void							VMUL();
	static void							VMAX();
	static void							VSUB();
	static void							VOPMSUB();
	static void							VMINI();
	static void							VX0();
	static void							VX1();
	static void							VX2();
	static void							VX3();

	//Vx (Common)
    static void                         VADDAbc();
    static void                         VSUBAbc();
	static void							VMULAbc();
	static void							VMADDAbc();
    static void                         VMSUBAbc();

	//V0
	static void							VITOF0();
	static void							VFTOI0();
    static void                         VADDA();
	static void							VMOVE();
	static void							VDIV();
    static void                         VRNEXT();

	//V1
    static void                         VITOF4();
	static void							VFTOI4();
	static void							VMR32();
	static void							VSQRT();

	//V2
	static void							VOPMULA();
	static void							VRSQRT();
	static void							VRINIT();

	//V3
    static void                         VITOF15();
    static void                         VCLIP();
	static void							VNOP();
	static void							VWAITQ();
	static void							VRXOR();

	static uint8						m_nBc;
	static uint8						m_nDest;
	static uint8						m_nFSF;
	static uint8						m_nFTF;

	static uint8						m_nFS;
	static uint8						m_nFT;
	static uint8						m_nFD;

	//Reflection tables
	static MIPSReflection::INSTRUCTION	m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION	m_cReflCop2[32];
	static MIPSReflection::INSTRUCTION	m_cReflV[64];
	static MIPSReflection::INSTRUCTION	m_cReflVX0[32];
	static MIPSReflection::INSTRUCTION	m_cReflVX1[32];
	static MIPSReflection::INSTRUCTION	m_cReflVX2[32];
	static MIPSReflection::INSTRUCTION	m_cReflVX3[32];

};

extern CCOP_VU g_COPVU;

#endif
