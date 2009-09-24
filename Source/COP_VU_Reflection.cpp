#include <string.h>
#include <stdio.h>
#include <boost/static_assert.hpp>
#include "COP_VU.h"
#include "VUShared.h"
#include "MIPS.h"

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
	uint8 nRT, nFD;

	nRT = (uint8)((nOpcode >> 16) & 0x001F);
	nFD = (uint8)((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, VF%i", CMIPS::m_sGPRName[nRT], nFD);
}

void CCOP_VU::ReflOpRtId(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRT, nID;

	nRT = (uint8)((nOpcode >> 16) & 0x001F);
	nID = (uint8)((nOpcode >> 11) & 0x001F);

	sprintf(sText, "%s, VI%i", CMIPS::m_sGPRName[nRT], nID);
}

void CCOP_VU::ReflOpAccFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void CCOP_VU::ReflOpFtOffRs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nRS, nFT;
	uint16 nImm;

	nRS  = (uint8) ((nOpcode >> 21) & 0x001F);
	nFT  = (uint8) ((nOpcode >> 16) & 0x001F);
	nImm = (uint16)((nOpcode >>  0) & 0xFFFF);

	sprintf(sText, "VF%i, $%0.4X(%s)", nFT, nImm, CMIPS::m_sGPRName[nRS]);
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
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x08
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMADD",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
    {	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x10
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	"VADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMUL",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"VMAX",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"VSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VOPMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"VMINI",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	//0x30
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"Vx0",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"Vx1",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"Vx2",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	"Vx3",		NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
};

INSTRUCTION CCOP_VU::m_cReflVX0[32] =
{
	//0x00
	{	"VADDA",	NULL,			CopyMnemonic,	    ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
    {	"VADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMOVE",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VDIV",		NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	"VADDA",	NULL,			CopyMnemonic,	    ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VFTOI4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMR32",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VSQRT",	NULL,			CopyMnemonic,		ReflOpQFtf,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	"VADDA",	NULL,			CopyMnemonic,	    ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF12",	NULL,			CopyMnemonic,		ReflOpFtFs,	    	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VOPMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VRSQRT",	NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	"VADDA",	NULL,			CopyMnemonic,	    ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VMSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VITOF15",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"VCLIP",	NULL,			CopyMnemonic,		ReflOpClip, 		NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"VNOP",		NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"WAITQ",	NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
    BOOST_STATIC_ASSERT(sizeof(m_ReflGeneral)   == sizeof(m_cReflGeneral));
    BOOST_STATIC_ASSERT(sizeof(m_ReflCop2)      == sizeof(m_cReflCop2));
    BOOST_STATIC_ASSERT(sizeof(m_ReflV)         == sizeof(m_cReflV));
    BOOST_STATIC_ASSERT(sizeof(m_ReflVX0)       == sizeof(m_cReflVX0));
    BOOST_STATIC_ASSERT(sizeof(m_ReflVX1)       == sizeof(m_cReflVX1));
    BOOST_STATIC_ASSERT(sizeof(m_ReflVX2)       == sizeof(m_cReflVX2));
    BOOST_STATIC_ASSERT(sizeof(m_ReflVX3)       == sizeof(m_cReflVX3));

    memcpy(m_ReflGeneral,   m_cReflGeneral, sizeof(m_cReflGeneral));
	memcpy(m_ReflCop2,      m_cReflCop2,    sizeof(m_cReflCop2));
	memcpy(m_ReflV,         m_cReflV,       sizeof(m_cReflV));
	memcpy(m_ReflVX0,       m_cReflVX0,     sizeof(m_cReflVX0));
	memcpy(m_ReflVX1,       m_cReflVX1,     sizeof(m_cReflVX1));
	memcpy(m_ReflVX2,       m_cReflVX2,     sizeof(m_cReflVX2));
	memcpy(m_ReflVX3,       m_cReflVX3,     sizeof(m_cReflVX3));

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
	INSTRUCTION Instr;
	unsigned int nCount;
	CMIPS* pCtx;

	nCount = 256;
	pCtx = NULL;

	if(nOpcode == 0)
	{
		strncpy(sText, "NOP", nCount);
		return;
	}

	Instr.pGetMnemonic	= SubTableMnemonic;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetMnemonic(&Instr, pCtx, nOpcode, sText, nCount);
}

void CCOP_VU::GetArguments(uint32 nAddress, uint32 nOpcode, char* sText)
{
	INSTRUCTION Instr;
	unsigned int nCount;
	CMIPS* pCtx;

	nCount = 256;
	pCtx = NULL;

	if(nOpcode == 0)
	{
		strncpy(sText, "", nCount);
		return;
	}

	Instr.pGetOperands	= SubTableOperands;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetOperands(&Instr, pCtx, nAddress, nOpcode, sText, 256);	
}

bool CCOP_VU::IsBranch(uint32 nOpcode)
{
	INSTRUCTION Instr;
	CMIPS* pCtx;

	pCtx = NULL;

	if(nOpcode == 0) return false;

	Instr.pIsBranch		= SubTableIsBranch;
	Instr.pSubTable		= &m_ReflGeneralTable;
	return Instr.pIsBranch(&Instr, pCtx, nOpcode);
}

uint32 CCOP_VU::GetEffectiveAddress(uint32 nAddress, uint32 nOpcode)
{
	INSTRUCTION Instr;
	CMIPS* pCtx;

	pCtx = NULL;

	if(nOpcode == 0) return 0;

	Instr.pGetEffectiveAddress	= SubTableEffAddr;
	Instr.pSubTable				= &m_ReflGeneralTable;
	return Instr.pGetEffectiveAddress(&Instr, pCtx, nAddress, nOpcode);
}
