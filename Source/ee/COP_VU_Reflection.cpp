#include <string.h>
#include <stdio.h>
#include "../MIPS.h"
#include "COP_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CCOP_VU::ReflMnemI(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nOpcode, char* sText, unsigned int nCount)
{
	strncpy(sText, pInstr->sMnemonic, nCount);

	if(nOpcode & 1)
	{
		strcat(sText, ".I");
	}
	else
	{
		strcat(sText, ".NI");
	}
}

void CCOP_VU::ReflOpRtFd(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFD = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, VF%i", CMIPS::m_sGPRName[nRT], nFD);
}

void CCOP_VU::ReflOpRtId(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nID = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, VI%i", CMIPS::m_sGPRName[nRT], nID);
}

void CCOP_VU::ReflOpImm15(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint16 nImm	= static_cast<uint16>((nOpcode >> 6) & 0x7FFF);

	sprintf(sText, "$%0.4X", nImm);
}

void CCOP_VU::ReflOpAccFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void CCOP_VU::ReflOpFtOffRs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRS	= static_cast<uint8> ((nOpcode >> 21) & 0x001F);
	uint8 nFT	= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint16 nImm = static_cast<uint16>((nOpcode >>  0) & 0xFFFF);

	sprintf(sText, "VF%i, $%0.4X(%s)", nFT, nImm, CMIPS::m_sGPRName[nRS]);
}

void CCOP_VU::ReflOpVi27(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	sprintf(sText, "VI27");
}

INSTRUCTION CCOP_VU::m_cReflGeneral[64] =
{
	//0x00
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"COP2",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x30
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"LQC2",		NULL,			CopyMnemonic,		ReflOpFtOffRs,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"SQC2",		NULL,			CopyMnemonic,		ReflOpFtOffRs,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_VU::m_cReflCop2[32] =
{
	//0x00
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"QMFC2",	NULL,			ReflMnemI,			ReflOpRtFd,			NULL,				NULL			},
	{	"CFC2",		NULL,			ReflMnemI,			ReflOpRtId,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"QMTC2",	NULL,			ReflMnemI,			ReflOpRtFd,			NULL,				NULL			},
	{	"CTC2",		NULL,			ReflMnemI,			ReflOpRtId,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	//0x18
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"V",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
};

INSTRUCTION CCOP_VU::m_cReflV[64] =
{
	//0x00
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	//0x08
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	//0x10
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	//0x18
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,						NULL			},
	//0x20
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,						NULL			},
	//0x28
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VOPMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,						NULL			},
	//0x30
	{	"VIADD",	NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,						NULL			},
	{	"VISUB",	NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,						NULL			},
	{	"VIADDI",	NULL,			CopyMnemonic,		ReflOpItIsImm5,		NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	"VIAND",	NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,						NULL			},
	{	"VIOR",		NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	//0x38
	{	"VCALLMS",	NULL,			CopyMnemonic,		ReflOpImm15,		IsNoDelayBranch,			NULL			},
	{	"VCALLMSR",	NULL,			CopyMnemonic,		ReflOpVi27,			IsNoDelayBranch,			NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,						NULL			},
	{	"Vx0",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,			SubTableEffAddr	},
	{	"Vx1",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,			SubTableEffAddr	},
	{	"Vx2",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,			SubTableEffAddr	},
	{	"Vx3",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,			SubTableEffAddr	},
};

INSTRUCTION CCOP_VU::m_cReflVX0[32] =
{
	//0x00
	{	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsQ,		NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"VMOVE",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VLQI",		NULL,			CopyMnemonic,		ReflOpFtDstIsInc,	NULL,				NULL			},
	{	"VDIV",		NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	"VMTIR",	NULL,			CopyMnemonic,		ReflOpItFsf,		NULL,				NULL			},
	//0x10
	{	"VRNEXT",	NULL,			CopyMnemonic,		ReflOpFtR,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_VU::m_cReflVX1[32] =
{
	//0x00
	{	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VABS",		NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"VMR32",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VSQI",		NULL,			CopyMnemonic,		ReflOpFsDstItInc,	NULL,				NULL			},
	{	"VSQRT",	NULL,			CopyMnemonic,		ReflOpQFtf,			NULL,				NULL			},
	{	"VMFIR",	NULL,			CopyMnemonic,		ReflOpFtIs,			NULL,				NULL			},
	//0x10
	{	"VRGET",	NULL,			CopyMnemonic,		ReflOpFtR,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_VU::m_cReflVX2[32] =
{
	//0x00
	{	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"VOPMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VRSQRT",	NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	"VILWR",	NULL,			CopyMnemonic,		ReflOpItIsDst,		NULL,				NULL			},
	//0x10
	{	"VRINIT",	NULL,			CopyMnemonic,		ReflOpRFsf,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CCOP_VU::m_cReflVX3[32] =
{
	//0x00
	{	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF15",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI15",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VCLIP",	NULL,			CopyMnemonic,		ReflOpClip, 		NULL,				NULL			},
	//0x08
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VNOP",		NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VSQD",		NULL,			CopyMnemonic,		ReflOpFsDstItDec,	NULL,				NULL			},
	{	"VWAITQ",	NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	"VISWR",	NULL,			CopyMnemonic,		ReflOpItIsDst,		NULL,				NULL			},
	//0x10
	{	"VRXOR",	NULL,			CopyMnemonic,		ReflOpRFsf,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

void CCOP_VU::SetupReflectionTables()
{
	static_assert(sizeof(m_ReflGeneral)	== sizeof(m_cReflGeneral),	"Array sizes don't match");
	static_assert(sizeof(m_ReflCop2)	== sizeof(m_cReflCop2),		"Array sizes don't match");
	static_assert(sizeof(m_ReflV)		== sizeof(m_cReflV),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX0)		== sizeof(m_cReflVX0),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX1)		== sizeof(m_cReflVX1),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX2)		== sizeof(m_cReflVX2),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX3)		== sizeof(m_cReflVX3),		"Array sizes don't match");

	memcpy(m_ReflGeneral,	m_cReflGeneral,	sizeof(m_cReflGeneral));
	memcpy(m_ReflCop2,		m_cReflCop2,	sizeof(m_cReflCop2));
	memcpy(m_ReflV,			m_cReflV,		sizeof(m_cReflV));
	memcpy(m_ReflVX0,		m_cReflVX0,		sizeof(m_cReflVX0));
	memcpy(m_ReflVX1,		m_cReflVX1,		sizeof(m_cReflVX1));
	memcpy(m_ReflVX2,		m_cReflVX2,		sizeof(m_cReflVX2));
	memcpy(m_ReflVX3,		m_cReflVX3,		sizeof(m_cReflVX3));

	m_ReflGeneralTable.nShift		= 26;
	m_ReflGeneralTable.nMask		= 0x3F;
	m_ReflGeneralTable.pTable		= m_ReflGeneral;

	m_ReflCop2Table.nShift			= 21;
	m_ReflCop2Table.nMask			= 0x1F;
	m_ReflCop2Table.pTable			= m_ReflCop2;

	m_ReflVTable.nShift				= 0;
	m_ReflVTable.nMask				= 0x3F;
	m_ReflVTable.pTable				= m_ReflV;

	m_ReflVX0Table.nShift			= 6;
	m_ReflVX0Table.nMask			= 0x1F;
	m_ReflVX0Table.pTable			= m_ReflVX0;

	m_ReflVX1Table.nShift			= 6;
	m_ReflVX1Table.nMask			= 0x1F;
	m_ReflVX1Table.pTable			= m_ReflVX1;

	m_ReflVX2Table.nShift			= 6;
	m_ReflVX2Table.nMask			= 0x1F;
	m_ReflVX2Table.pTable			= m_ReflVX2;

	m_ReflVX3Table.nShift			= 6;
	m_ReflVX3Table.nMask			= 0x1F;
	m_ReflVX3Table.pTable			= m_ReflVX3;

	m_ReflGeneral[0x12].pSubTable	= &m_ReflCop2Table;

	for(unsigned int i = 0x10; i < 0x20; i++)
	{
		m_ReflCop2[i].pSubTable		= &m_ReflVTable;
	}

	m_ReflV[0x3C].pSubTable			= &m_ReflVX0Table;
	m_ReflV[0x3D].pSubTable			= &m_ReflVX1Table;
	m_ReflV[0x3E].pSubTable			= &m_ReflVX2Table;
	m_ReflV[0x3F].pSubTable			= &m_ReflVX3Table;
}

void CCOP_VU::GetInstruction(uint32 nOpcode, char* sText)
{
	unsigned int nCount = 256;

	if(nOpcode == 0)
	{
		strncpy(sText, "NOP", nCount);
		return;
	}

	CMIPS* pCtx = NULL;

	INSTRUCTION Instr;
	Instr.pGetMnemonic	= SubTableMnemonic;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetMnemonic(&Instr, pCtx, nOpcode, sText, nCount);
}

void CCOP_VU::GetArguments(uint32 nAddress, uint32 nOpcode, char* sText)
{
	unsigned int nCount = 256;

	if(nOpcode == 0)
	{
		strncpy(sText, "", nCount);
		return;
	}

	CMIPS* pCtx = NULL;

	INSTRUCTION Instr;
	Instr.pGetOperands	= SubTableOperands;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetOperands(&Instr, pCtx, nAddress, nOpcode, sText, 256);	
}

MIPS_BRANCH_TYPE CCOP_VU::IsBranch(uint32 nOpcode)
{
	if(nOpcode == 0) return MIPS_BRANCH_NONE;

	CMIPS* pCtx = NULL;

	INSTRUCTION Instr;
	Instr.pIsBranch		= SubTableIsBranch;
	Instr.pSubTable		= &m_ReflGeneralTable;
	return Instr.pIsBranch(&Instr, pCtx, nOpcode);
}

uint32 CCOP_VU::GetEffectiveAddress(uint32 nAddress, uint32 nOpcode)
{
	if(nOpcode == 0) return 0;

	CMIPS* pCtx = NULL;

	INSTRUCTION Instr;
	Instr.pGetEffectiveAddress	= SubTableEffAddr;
	Instr.pSubTable				= &m_ReflGeneralTable;
	return Instr.pGetEffectiveAddress(&Instr, pCtx, nAddress, nOpcode);
}
