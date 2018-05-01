#pragma once

#include "../MIPSCoprocessor.h"
#include "../MIPSReflection.h"
#include "../Ps2Const.h"

class CCOP_VU : public CMIPSCoprocessor
{
public:
	CCOP_VU(MIPS_REGSIZE);

	void CompileInstruction(uint32, CMipsJitter*, CMIPS*) override;
	void GetInstruction(uint32, char*) override;
	void GetArguments(uint32, uint32, char*) override;
	uint32 GetEffectiveAddress(uint32, uint32) override;
	MIPS_BRANCH_TYPE IsBranch(uint32) override;

protected:
	typedef void (CCOP_VU::*InstructionFuncConstant)();

	void SetupReflectionTables();

	static void ReflMnemI(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, char*, unsigned int);

	static void ReflOpOff(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpRtFd(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpRtId(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpImm15(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpAccFsFt(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpFtOffRs(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);
	static void ReflOpVi27(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32, char*, unsigned int);

	static uint32 ReflEaOffset(MIPSReflection::INSTRUCTION*, CMIPS*, uint32, uint32);

	MIPSReflection::INSTRUCTION m_ReflGeneral[64];
	MIPSReflection::INSTRUCTION m_ReflCop2[32];
	MIPSReflection::INSTRUCTION m_ReflBc2[4];
	MIPSReflection::INSTRUCTION m_ReflV[64];
	MIPSReflection::INSTRUCTION m_ReflVX0[32];
	MIPSReflection::INSTRUCTION m_ReflVX1[32];
	MIPSReflection::INSTRUCTION m_ReflVX2[32];
	MIPSReflection::INSTRUCTION m_ReflVX3[32];

	MIPSReflection::SUBTABLE m_ReflGeneralTable;
	MIPSReflection::SUBTABLE m_ReflCop2Table;
	MIPSReflection::SUBTABLE m_ReflBc2Table;
	MIPSReflection::SUBTABLE m_ReflVTable;
	MIPSReflection::SUBTABLE m_ReflVX0Table;
	MIPSReflection::SUBTABLE m_ReflVX1Table;
	MIPSReflection::SUBTABLE m_ReflVX2Table;
	MIPSReflection::SUBTABLE m_ReflVX3Table;

	static InstructionFuncConstant m_pOpCop2[0x20];
	static InstructionFuncConstant m_pOpVector[0x40];
	static InstructionFuncConstant m_pOpVx0[0x20];
	static InstructionFuncConstant m_pOpVx1[0x20];
	static InstructionFuncConstant m_pOpVx2[0x20];
	static InstructionFuncConstant m_pOpVx3[0x20];

private:
	//General
	void LQC2();
	void SQC2();

	//COP2
	void QMFC2();
	void CFC2();
	void QMTC2();
	void CTC2();
	void BC2();
	void V();

	//Vector
	void VADDbc();
	void VSUBbc();
	void VMADDbc();
	void VMSUBbc();
	void VMAXbc();
	void VMINIbc();
	void VMULbc();
	void VMULq();
	void VMAXi();
	void VMULi();
	void VMINIi();
	void VADDq();
	void VMADDq();
	void VADDi();
	void VMADDi();
	void VSUBq();
	void VMSUBq();
	void VSUBi();
	void VMSUBi();
	void VADD();
	void VMADD();
	void VMUL();
	void VMAX();
	void VSUB();
	void VMSUB();
	void VOPMSUB();
	void VMINI();
	void VIADD();
	void VISUB();
	void VIADDI();
	void VIAND();
	void VIOR();
	void VCALLMS();
	void VCALLMSR();
	void VX0();
	void VX1();
	void VX2();
	void VX3();

	//Vx (Common)
	void VADDAbc();
	void VSUBAbc();
	void VMULAbc();
	void VMULAq();
	void VMADDAbc();
	void VMSUBAbc();

	//V0
	void VITOF0();
	void VFTOI0();
	void VADDA();
	void VSUBA();
	void VMOVE();
	void VLQI();
	void VDIV();
	void VMTIR();
	void VRNEXT();

	//V1
	void VITOF4();
	void VFTOI4();
	void VABS();
	void VMADDAq();
	void VMSUBAq();
	void VMADDA();
	void VMSUBA();
	void VMR32();
	void VSQI();
	void VSQRT();
	void VMFIR();
	void VRGET();

	//V2
	void VITOF12();
	void VFTOI12();
	void VMULAi();
	void VMULA();
	void VOPMULA();
	void VLQD();
	void VRSQRT();
	void VILWR();
	void VRINIT();

	//V3
	void VITOF15();
	void VFTOI15();
	void VCLIP();
	void VMADDAi();
	void VMSUBAi();
	void VNOP();
	void VSQD();
	void VWAITQ();
	void VISWR();
	void VRXOR();

	uint8 m_nBc = 0;
	uint8 m_nDest = 0;
	uint8 m_nFSF = 0;
	uint8 m_nFTF = 0;

	uint8 m_nFS = 0;
	uint8 m_nFT = 0;
	uint8 m_nFD = 0;

	uint8 m_nIT = 0;
	uint8 m_nIS = 0;
	uint8 m_nID = 0;
	uint8 m_nImm5 = 0;
	uint16 m_nImm15 = 0;
	static const uint32 m_vuMemAddressMask = (PS2::VUMEM0SIZE - 1);

	//Reflection tables
	static MIPSReflection::INSTRUCTION m_cReflGeneral[64];
	static MIPSReflection::INSTRUCTION m_cReflCop2[32];
	static MIPSReflection::INSTRUCTION m_cReflBc2[4];
	static MIPSReflection::INSTRUCTION m_cReflV[64];
	static MIPSReflection::INSTRUCTION m_cReflVX0[32];
	static MIPSReflection::INSTRUCTION m_cReflVX1[32];
	static MIPSReflection::INSTRUCTION m_cReflVX2[32];
	static MIPSReflection::INSTRUCTION m_cReflVX3[32];
};
