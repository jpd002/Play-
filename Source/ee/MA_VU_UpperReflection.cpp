#include <string.h>
#include <stdio.h>
#include "MA_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CMA_VU::CUpper::ReflOpFtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nDest = static_cast<uint8>((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

// clang-format off
INSTRUCTION CMA_VU::CUpper::m_cReflV[64] =
{
	//0x00
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x08
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x10
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x18
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	//0x20
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	//0x28
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"OPMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
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

INSTRUCTION CMA_VU::CUpper::m_cReflVX0[32] =
{
	//0x00
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsQ,		NULL,				NULL			},
	//0x08
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsQ,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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

INSTRUCTION CMA_VU::CUpper::m_cReflVX1[32] =
{
	//0x00
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ABS",		NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	//0x08
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsQ,		NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsQ,		NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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

INSTRUCTION CMA_VU::CUpper::m_cReflVX2[32] =
{
	//0x00
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	//0x08
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	"OPMULA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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

INSTRUCTION CMA_VU::CUpper::m_cReflVX3[32] =
{
	//0x00
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"SUBA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF15",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI15",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"CLIP",		NULL,			CopyMnemonic,		ReflOpClip,			NULL,				NULL			},
	//0x08
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"NOP",		NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflV[64] =
{
	//0x00
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	//0x08
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	//0x10
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsFt		},
	//0x18
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsQ	},
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsI		},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsI	},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsI		},
	//0x20
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsQ	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsQ	},
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsI	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsI	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsQ	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsQ	},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsI	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsI	},
	//0x28
	{	"ADD",		NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"MADD",		NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"MUL",		NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"MAX",		NULL,			ReflOpAffWrFdRdFsFt		},
	{	"SUB",		NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"MSUB",		NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"OPMSUB",	NULL,			ReflOpAffWrFdMfRdFsFt	},
	{	"MINI",		NULL,			ReflOpAffWrFdRdFsFt		},
	//0x30
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x38
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"Vx0",		NULL,			SubTableAffectedOperands },
	{	"Vx1",		NULL,			SubTableAffectedOperands },
	{	"Vx2",		NULL,			SubTableAffectedOperands },
	{	"Vx3",		NULL,			SubTableAffectedOperands },
};

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX0[32] =
{
	//0x00
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"ITOF0",	NULL,			ReflOpAffWrFtRdFs		},
	{	"FTOI0",	NULL,			ReflOpAffWrFtRdFs		},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsFtBc	},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsQ		},
	//0x08
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsQ		},
	{	NULL,		NULL,			NULL					},
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	//0x10
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX1[32] =
{
	//0x00
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"ITOF4",	NULL,			ReflOpAffWrFtRdFs		},
	{	"FTOI4",	NULL,			ReflOpAffWrFtRdFs		},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsFtBc	},
	{	"ABS",		NULL,			ReflOpAffWrFtRdFs		},
	//0x08
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsQ		},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsQ		},
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	//0x10
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX2[32] =
{
	//0x00
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"ITOF12",	NULL,			ReflOpAffWrFtRdFs		},
	{	"FTOI12",	NULL,			ReflOpAffWrFtRdFs		},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsFtBc	},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsI		},
	//0x08
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsI		},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsI		},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"OPMULA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	{	NULL,		NULL,			NULL					},
	//0x10
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX3[32] =
{
	//0x00
	{	"ADDA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"SUBA",		NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsFt	},
	{	"ITOF15",	NULL,			ReflOpAffWrFtRdFs		},
	{	"FTOI15",	NULL,			ReflOpAffWrFtRdFs		},
	{	"MULA",		NULL,			ReflOpAffWrAMfRdFsFtBc	},
	{	"CLIP",		NULL,			ReflOpAffWrCfRdFsFt		},
	//0x08
	{	"MADDA",	NULL,			ReflOpAffWrAMfRdFsI	},
	{	"MSUBA",	NULL,			ReflOpAffWrAMfRdFsI	},
	{	NULL,		NULL,			NULL				},
	{	"NOP",		NULL,			ReflOpAffNone		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x10
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};
// clang-format on

void CMA_VU::CUpper::SetupReflectionTables()
{
	static_assert(sizeof(m_ReflV) == sizeof(m_cReflV), "Array sizes don't match");
	static_assert(sizeof(m_ReflVX0) == sizeof(m_cReflVX0), "Array sizes don't match");
	static_assert(sizeof(m_ReflVX1) == sizeof(m_cReflVX1), "Array sizes don't match");
	static_assert(sizeof(m_ReflVX2) == sizeof(m_cReflVX2), "Array sizes don't match");
	static_assert(sizeof(m_ReflVX3) == sizeof(m_cReflVX3), "Array sizes don't match");

	static_assert(sizeof(m_VuReflV) == sizeof(m_cVuReflV), "Array sizes don't match");
	static_assert(sizeof(m_VuReflVX0) == sizeof(m_cVuReflVX0), "Array sizes don't match");
	static_assert(sizeof(m_VuReflVX1) == sizeof(m_cVuReflVX1), "Array sizes don't match");
	static_assert(sizeof(m_VuReflVX2) == sizeof(m_cVuReflVX2), "Array sizes don't match");
	static_assert(sizeof(m_VuReflVX3) == sizeof(m_cVuReflVX3), "Array sizes don't match");

	VerifyVuReflectionTable(m_cReflV, m_cVuReflV, sizeof(m_cReflV) / sizeof(m_cReflV[0]));
	VerifyVuReflectionTable(m_cReflVX0, m_cVuReflVX0, sizeof(m_cReflVX0) / sizeof(m_cReflVX0[0]));
	VerifyVuReflectionTable(m_cReflVX1, m_cVuReflVX1, sizeof(m_cReflVX1) / sizeof(m_cReflVX1[0]));
	VerifyVuReflectionTable(m_cReflVX2, m_cVuReflVX2, sizeof(m_cReflVX2) / sizeof(m_cReflVX2[0]));
	VerifyVuReflectionTable(m_cReflVX3, m_cVuReflVX3, sizeof(m_cReflVX3) / sizeof(m_cReflVX3[0]));

	memcpy(m_ReflV, m_cReflV, sizeof(m_cReflV));
	memcpy(m_ReflVX0, m_cReflVX0, sizeof(m_cReflVX0));
	memcpy(m_ReflVX1, m_cReflVX1, sizeof(m_cReflVX1));
	memcpy(m_ReflVX2, m_cReflVX2, sizeof(m_cReflVX2));
	memcpy(m_ReflVX3, m_cReflVX3, sizeof(m_cReflVX3));

	memcpy(m_VuReflV, m_cVuReflV, sizeof(m_cVuReflV));
	memcpy(m_VuReflVX0, m_cVuReflVX0, sizeof(m_cVuReflVX0));
	memcpy(m_VuReflVX1, m_cVuReflVX1, sizeof(m_cVuReflVX1));
	memcpy(m_VuReflVX2, m_cVuReflVX2, sizeof(m_cVuReflVX2));
	memcpy(m_VuReflVX3, m_cVuReflVX3, sizeof(m_cVuReflVX3));

	//Standard Tables
	{
		m_ReflVTable.nShift = 0;
		m_ReflVTable.nMask = 0x3F;
		m_ReflVTable.pTable = m_ReflV;

		m_ReflVX0Table.nShift = 6;
		m_ReflVX0Table.nMask = 0x1F;
		m_ReflVX0Table.pTable = m_ReflVX0;

		m_ReflVX1Table.nShift = 6;
		m_ReflVX1Table.nMask = 0x1F;
		m_ReflVX1Table.pTable = m_ReflVX1;

		m_ReflVX2Table.nShift = 6;
		m_ReflVX2Table.nMask = 0x1F;
		m_ReflVX2Table.pTable = m_ReflVX2;

		m_ReflVX3Table.nShift = 6;
		m_ReflVX3Table.nMask = 0x1F;
		m_ReflVX3Table.pTable = m_ReflVX3;

		m_ReflV[0x3C].pSubTable = &m_ReflVX0Table;
		m_ReflV[0x3D].pSubTable = &m_ReflVX1Table;
		m_ReflV[0x3E].pSubTable = &m_ReflVX2Table;
		m_ReflV[0x3F].pSubTable = &m_ReflVX3Table;
	}

	//VU extended tables
	{
		m_VuReflVTable.nShift = 0;
		m_VuReflVTable.nMask = 0x3F;
		m_VuReflVTable.pTable = m_VuReflV;

		m_VuReflVX0Table.nShift = 6;
		m_VuReflVX0Table.nMask = 0x1F;
		m_VuReflVX0Table.pTable = m_VuReflVX0;

		m_VuReflVX1Table.nShift = 6;
		m_VuReflVX1Table.nMask = 0x1F;
		m_VuReflVX1Table.pTable = m_VuReflVX1;

		m_VuReflVX2Table.nShift = 6;
		m_VuReflVX2Table.nMask = 0x1F;
		m_VuReflVX2Table.pTable = m_VuReflVX2;

		m_VuReflVX3Table.nShift = 6;
		m_VuReflVX3Table.nMask = 0x1F;
		m_VuReflVX3Table.pTable = m_VuReflVX3;

		m_VuReflV[0x3C].subTable = &m_VuReflVX0Table;
		m_VuReflV[0x3D].subTable = &m_VuReflVX1Table;
		m_VuReflV[0x3E].subTable = &m_VuReflVX2Table;
		m_VuReflV[0x3F].subTable = &m_VuReflVX3Table;
	}
}

void CMA_VU::CUpper::GetInstructionMnemonic(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	INSTRUCTION Instr;
	Instr.pGetMnemonic = SubTableMnemonic;
	Instr.pSubTable = &m_ReflVTable;
	Instr.pGetMnemonic(&Instr, pCtx, nOpcode, sText, nCount);
}

void CMA_VU::CUpper::GetInstructionOperands(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	INSTRUCTION Instr;
	Instr.pGetOperands = SubTableOperands;
	Instr.pSubTable = &m_ReflVTable;
	Instr.pGetOperands(&Instr, pCtx, nAddress, nOpcode, sText, nCount);
}

MIPS_BRANCH_TYPE CMA_VU::CUpper::IsInstructionBranch(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	INSTRUCTION Instr;
	Instr.pIsBranch = SubTableIsBranch;
	Instr.pSubTable = &m_ReflVTable;
	return Instr.pIsBranch(&Instr, pCtx, nOpcode);
}

uint32 CMA_VU::CUpper::GetInstructionEffectiveAddress(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	INSTRUCTION Instr;
	Instr.pGetEffectiveAddress = SubTableEffAddr;
	Instr.pSubTable = &m_ReflVTable;
	return Instr.pGetEffectiveAddress(&Instr, pCtx, nAddress, nOpcode);
}

VUShared::OPERANDSET CMA_VU::CUpper::GetAffectedOperands(CMIPS* context, uint32 address, uint32 opcode)
{
	OPERANDSET result;
	memset(&result, 0, sizeof(OPERANDSET));

	VUINSTRUCTION instr;
	instr.pGetAffectedOperands = SubTableAffectedOperands;
	instr.subTable = &m_VuReflVTable;
	instr.pGetAffectedOperands(&instr, context, address, opcode, result);
	return result;
}
