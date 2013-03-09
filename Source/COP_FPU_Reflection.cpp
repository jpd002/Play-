#include <string.h>
#include <stdio.h>
#include "COP_FPU.h"
#include "MIPS.h"

using namespace MIPSReflection;

void CCOP_FPU::ReflOpRtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, F%i", CMIPS::m_sGPRName[nRT], nFS);
}

void CCOP_FPU::ReflOpRtFcs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, FCR%i", CMIPS::m_sGPRName[nRT], nFS);
}

void CCOP_FPU::ReflOpFdFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFD = static_cast<uint8>((nOpcode >>  6) & 0x001F);

	sprintf(sText, "F%i, F%i", nFD, nFS);
}

void CCOP_FPU::ReflOpFdFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFD = static_cast<uint8>((nOpcode >>  6) & 0x001F);

	sprintf(sText, "F%i, F%i", nFD, nFT);
}

void CCOP_FPU::ReflOpFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFT = static_cast<uint8>((nOpcode >> 16) & 0x001F);

	sprintf(sText, "F%i, F%i", nFS, nFT);
}

void CCOP_FPU::ReflOpCcFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nCC = static_cast<uint8>((nOpcode >>  8) & 0x0007);

	sprintf(sText, "CC%i, F%i, F%i", nCC, nFS, nFT);
}

void CCOP_FPU::ReflOpFdFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFD = static_cast<uint8>((nOpcode >>  6) & 0x001F);

	sprintf(sText, "F%i, F%i, F%i", nFD, nFS, nFT);
}

void CCOP_FPU::ReflOpFtOffRs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8  nRS  = static_cast<uint8> ((nOpcode >> 21) & 0x001F);
	uint8  nFT  = static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint16 nImm = static_cast<uint16>((nOpcode >>  0) & 0xFFFF);

	sprintf(sText, "F%i, $%0.4X(%s)", nFT, nImm, CMIPS::m_sGPRName[nRS]);
}

void CCOP_FPU::ReflOpCcOff(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint16 nImm = static_cast<uint16>((nOpcode >>  0) & 0xFFFF);
	nAddress += 4;
	sprintf(sText, "CC%i, $%0.8X", (nOpcode >> 18) & 0x07, nAddress + CMIPS::GetBranch(nImm));
}

uint32 CCOP_FPU::ReflEaOffset(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	uint16 nImm = static_cast<uint16>((nOpcode >>  0) & 0xFFFF);
	nAddress += 4;
	return (nAddress + CMIPS::GetBranch(nImm));
}

INSTRUCTION CCOP_FPU::m_cReflGeneral[64] =
{
	//0x00
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"COP1",			NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x30
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"LWC1",			NULL,			CopyMnemonic,		ReflOpFtOffRs,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"SWC1",			NULL,			CopyMnemonic,		ReflOpFtOffRs,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_FPU::m_cReflCop1[32] =
{
	//0x00
	{	"MFC1",			NULL,			CopyMnemonic,		ReflOpRtFs,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"CFC1",			NULL,			CopyMnemonic,		ReflOpRtFcs,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MTC1",			NULL,			CopyMnemonic,		ReflOpRtFs,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"CTC1",			NULL,			CopyMnemonic,		ReflOpRtFcs,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	"BC1",			NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	"S",			NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"W",			NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_FPU::m_cReflBc1[4] =
{
	//0x00
	{	"BC1F",			NULL,			CopyMnemonic,		ReflOpCcOff,		MIPSReflection::IsBranch,	ReflEaOffset	},
	{	"BC1T",			NULL,			CopyMnemonic,		ReflOpCcOff,		MIPSReflection::IsBranch,	ReflEaOffset	},
	{	"BC1FL",		NULL,			CopyMnemonic,		ReflOpCcOff,		MIPSReflection::IsBranch,	ReflEaOffset	},
	{	"BC1TL",		NULL,			CopyMnemonic,		ReflOpCcOff,		MIPSReflection::IsBranch,	ReflEaOffset	},
};

INSTRUCTION CCOP_FPU::m_cReflS[64] =
{
	//0x00
	{	"ADD.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"SUB.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MUL.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"DIV.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"SQRT.S",		NULL,			CopyMnemonic,		ReflOpFdFt,			NULL,				NULL			},
	{	"ABS.S",		NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	{	"MOV.S",		NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	{	"NEG.S",		NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	//0x08
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"TRUNC.W.S",	NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"RSQRT.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	"ADDA.S",		NULL,			CopyMnemonic,		ReflOpFsFt,			NULL,				NULL			},
	{	"SUBA.S",		NULL,			CopyMnemonic,		ReflOpFsFt,			NULL,				NULL			},
	{	"MULA.S",		NULL,			CopyMnemonic,		ReflOpFsFt,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADD.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MSUB.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"CVT.W.S",		NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	"MAX.S",		NULL,			CopyMnemonic,   	ReflOpFdFsFt,		NULL,				NULL			},
	{	"MIN.S",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x30
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"C.EQ.S",		NULL,			CopyMnemonic,		ReflOpCcFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"C.LT.S",		NULL,			CopyMnemonic,		ReflOpCcFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"C.LE.S",		NULL,			CopyMnemonic,		ReflOpCcFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"C.LT.S",		NULL,			CopyMnemonic,		ReflOpCcFsFt,		NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_FPU::m_cReflW[64] =
{
	//0x00
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	"CVT.S.W",		NULL,			CopyMnemonic,		ReflOpFdFs,			NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x30
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,			NULL,			NULL,				NULL,				NULL,				NULL			},
};

void CCOP_FPU::SetupReflectionTables()
{
	static_assert(sizeof(m_ReflGeneral)	== sizeof(m_cReflGeneral),	"Array sizes don't match");
	static_assert(sizeof(m_ReflCop1)	== sizeof(m_cReflCop1),		"Array sizes don't match");
	static_assert(sizeof(m_ReflBc1)		== sizeof(m_cReflBc1),		"Array sizes don't match");
	static_assert(sizeof(m_ReflS)		== sizeof(m_cReflS),		"Array sizes don't match");
	static_assert(sizeof(m_ReflW)		== sizeof(m_cReflW),		"Array sizes don't match");

	memcpy(m_ReflGeneral,	m_cReflGeneral,	sizeof(m_cReflGeneral));
	memcpy(m_ReflCop1,		m_cReflCop1,	sizeof(m_cReflCop1));
	memcpy(m_ReflBc1,		m_cReflBc1,		sizeof(m_cReflBc1));
	memcpy(m_ReflS,			m_cReflS,		sizeof(m_cReflS));
	memcpy(m_ReflW,			m_cReflW,		sizeof(m_cReflW));

	m_ReflGeneralTable.nShift	= 26;
	m_ReflGeneralTable.nMask	= 0x3F;
	m_ReflGeneralTable.pTable	= m_ReflGeneral;

	m_ReflCop1Table.nShift		= 21;
	m_ReflCop1Table.nMask		= 0x1F;
	m_ReflCop1Table.pTable		= m_ReflCop1;

	m_ReflBc1Table.nShift		= 16;
	m_ReflBc1Table.nMask		= 0x03;
	m_ReflBc1Table.pTable		= m_ReflBc1;

	m_ReflSTable.nShift			= 0;
	m_ReflSTable.nMask			= 0x3F;
	m_ReflSTable.pTable			= m_ReflS;

	m_ReflWTable.nShift			= 0;
	m_ReflWTable.nMask			= 0x3F;
	m_ReflWTable.pTable			= m_ReflW;

	m_ReflGeneral[0x11].pSubTable	= &m_ReflCop1Table;

	m_ReflCop1[0x08].pSubTable		= &m_ReflBc1Table;
	m_ReflCop1[0x10].pSubTable		= &m_ReflSTable;
	m_ReflCop1[0x14].pSubTable		= &m_ReflWTable;
}

void CCOP_FPU::GetInstruction(uint32 nOpcode, char* sText)
{
	unsigned int nCount = 256;
	if(nOpcode == 0)
	{
		strncpy(sText, "NOP", nCount);
		return;
	}

	INSTRUCTION Instr;
	Instr.pGetMnemonic	= SubTableMnemonic;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetMnemonic(&Instr, NULL, nOpcode, sText, nCount);
}

void CCOP_FPU::GetArguments(uint32 nAddress, uint32 nOpcode, char* sText)
{
	unsigned int nCount = 256;
	if(nOpcode == 0)
	{
		strncpy(sText, "", nCount);
		return;
	}

	INSTRUCTION Instr;
	Instr.pGetOperands	= SubTableOperands;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetOperands(&Instr, NULL, nAddress, nOpcode, sText, nCount);	
}

MIPS_BRANCH_TYPE CCOP_FPU::IsBranch(uint32 nOpcode)
{
	if(nOpcode == 0) return MIPS_BRANCH_NONE;

	INSTRUCTION Instr;
	Instr.pIsBranch		= SubTableIsBranch;
	Instr.pSubTable		= &m_ReflGeneralTable;
	return Instr.pIsBranch(&Instr, NULL, nOpcode);
}

uint32 CCOP_FPU::GetEffectiveAddress(uint32 nAddress, uint32 nOpcode)
{
	if(nOpcode == 0) return 0;

	INSTRUCTION Instr;
	Instr.pGetEffectiveAddress	= SubTableEffAddr;
	Instr.pSubTable				= &m_ReflGeneralTable;
	return Instr.pGetEffectiveAddress(&Instr, NULL, nAddress, nOpcode);
}
