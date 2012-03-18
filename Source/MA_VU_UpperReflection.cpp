#include <string.h>
#include <stdio.h>
#include <boost/static_assert.hpp>
#include "MA_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CMA_VU::CUpper::ReflOpFtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT		= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS		= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nDest		= static_cast<uint8>((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x18
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MINI",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	//0x20
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsQ,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	"MSUB",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	//0x28
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MUL",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"OPMSUB",	NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
	{	"MINI",	    NULL,			CopyMnemonic,		ReflOpFdFsFt,		NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ADDA",	    NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ABS",		NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFt,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"ITOF12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI12",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	//0x08
	{	"ADDA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	"MSUBA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	"ADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"ADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"SUB",		NULL,			ReflOpAffFdFsFtBc	},
	//0x08
	{	"MADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MADD",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MSUB",		NULL,			ReflOpAffFdFsFtBc	},
	//0x10
	{	"MAX",		NULL,			ReflOpAffFdFsFtBc	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MAX",		NULL,			ReflOpAffFdFsFtBc	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MINI",		NULL,			ReflOpAffFdFsFtBc	},
	//0x18
	{	"MUL",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffFdFsFtBc	},
	{	"MUL",		NULL,			ReflOpAffFdFsQ		},
	{	NULL,		NULL,			NULL				},
	{	"MUL",		NULL,			ReflOpAffFdFsI		},
	{	"MINI",		NULL,			ReflOpAffFdFsI		},
	//0x20
	{	"ADD",		NULL,			ReflOpAffFdFsQ		},
	{	"MADD",		NULL,			ReflOpAffFdFsQ		},
	{	"ADD",		NULL,			ReflOpAffFdFsI		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"SUB",		NULL,			ReflOpAffFdFsI		},
	{	"MSUB",		NULL,			ReflOpAffFdFsI		},
	//0x28
	{	"ADD",		NULL,			ReflOpAffFdFsFt		},
	{	"MADD",		NULL,			ReflOpAffFdFsFt		},
	{	"MUL",		NULL,			ReflOpAffFdFsFt		},
	{	"MAX",		NULL,			ReflOpAffFdFsFt		},
	{	"SUB",		NULL,			ReflOpAffFdFsFt		},
	{	NULL,		NULL,			NULL				},
	{	"OPMSUB",	NULL,			ReflOpAffFdFsFt		},
	{	"MINI",		NULL,			ReflOpAffFdFsFt		},
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
	{	"ADDA",		NULL,			ReflOpAffAccFsFtBc	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MSUBA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"ITOF0",	NULL,			ReflOpAffFtFs		},
	{	"FTOI0",	NULL,			ReflOpAffFtFs		},
	{	"MULA",		NULL,			ReflOpAffAccFsFtBc	},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ADDA",		NULL,			ReflOpAffAccFsFt	},
	{	NULL,		NULL,			NULL				},
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

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX1[32] =
{
	//0x00
	{	"ADDA",		NULL,			ReflOpAffAccFsFtBc	},
	{	NULL,		NULL,			NULL				},
	{	"MADDA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"MSUBA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"ITOF4",	NULL,			ReflOpAffFtFs,		},
	{	"FTOI4",	NULL,			ReflOpAffFtFs,		},
	{	"MULA",		NULL,			ReflOpAffAccFsFtBc	},
	{	"ABS",		NULL,			ReflOpAffFtFs		},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MADDA",	NULL,			ReflOpAffAccFsFt	},
	{	NULL,		NULL,			NULL				},
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

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX2[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MADDA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"MSUBA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"ITOF12",	NULL,			ReflOpAffFtFs		},
	{	"FTOI12",	NULL,			ReflOpAffFtFs		},
	{	"MULA",		NULL,			ReflOpAffAccFsFtBc	},
	{	"MULA",		NULL,			ReflOpAffAccFsI		},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MULA",		NULL,			ReflOpAffAccFsFt	},
	{	"OPMULA",	NULL,			ReflOpAffAccFsFt	},
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

VUINSTRUCTION CMA_VU::CUpper::m_cVuReflVX3[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MADDA",	NULL,			ReflOpAffAccFsFtBc	},
	{	"MSUBA",	NULL,			ReflOpAffAccFsFtBc	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MULA",		NULL,			ReflOpAffAccFsFtBc	},
	{	"CLIP",		NULL,			NULL				},
	//0x08
	{	"MADDA",	NULL,			ReflOpAffAccFsI		},
	{	"MSUBA",	NULL,			ReflOpAffAccFsI		},
	{	NULL,		NULL,			NULL				},
	{	"NOP",		NULL,			NULL				},
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

void CMA_VU::CUpper::SetupReflectionTables()
{
	static_assert(sizeof(m_ReflV)		== sizeof(m_cReflV),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX0)		== sizeof(m_cReflVX0),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX1)		== sizeof(m_cReflVX1),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX2)		== sizeof(m_cReflVX2),		"Array sizes don't match");
	static_assert(sizeof(m_ReflVX3)		== sizeof(m_cReflVX3),		"Array sizes don't match");

	static_assert(sizeof(m_VuReflV)		== sizeof(m_cVuReflV),		"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX0)	== sizeof(m_cVuReflVX0),	"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX1)	== sizeof(m_cVuReflVX1),	"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX2)	== sizeof(m_cVuReflVX2),	"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX3)	== sizeof(m_cVuReflVX3),	"Array sizes don't match");

	memcpy(m_ReflV,			m_cReflV,		sizeof(m_cReflV));
	memcpy(m_ReflVX0,		m_cReflVX0,		sizeof(m_cReflVX0));
	memcpy(m_ReflVX1,		m_cReflVX1,		sizeof(m_cReflVX1));
	memcpy(m_ReflVX2,		m_cReflVX2,		sizeof(m_cReflVX2));
	memcpy(m_ReflVX3,		m_cReflVX3,		sizeof(m_cReflVX3));

	memcpy(m_VuReflV,		m_cVuReflV,		sizeof(m_cVuReflV));
	memcpy(m_VuReflVX0,		m_cVuReflVX0,	sizeof(m_cVuReflVX0));
	memcpy(m_VuReflVX1,		m_cVuReflVX1,	sizeof(m_cVuReflVX1));
	memcpy(m_VuReflVX2,		m_cVuReflVX2,	sizeof(m_cVuReflVX2));
	memcpy(m_VuReflVX3,		m_cVuReflVX3,	sizeof(m_cVuReflVX3));

	//Standard Tables
	{
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

		m_ReflV[0x3C].pSubTable			= &m_ReflVX0Table;
		m_ReflV[0x3D].pSubTable			= &m_ReflVX1Table;
		m_ReflV[0x3E].pSubTable			= &m_ReflVX2Table;
		m_ReflV[0x3F].pSubTable			= &m_ReflVX3Table;
	}

	//VU extended tables
	{
		m_VuReflVTable.nShift			= 0;
		m_VuReflVTable.nMask			= 0x3F;
		m_VuReflVTable.pTable			= m_VuReflV;

		m_VuReflVX0Table.nShift			= 6;
		m_VuReflVX0Table.nMask			= 0x1F;
		m_VuReflVX0Table.pTable			= m_VuReflVX0;

		m_VuReflVX1Table.nShift			= 6;
		m_VuReflVX1Table.nMask			= 0x1F;
		m_VuReflVX1Table.pTable			= m_VuReflVX1;

		m_VuReflVX2Table.nShift			= 6;
		m_VuReflVX2Table.nMask			= 0x1F;
		m_VuReflVX2Table.pTable			= m_VuReflVX2;

		m_VuReflVX3Table.nShift			= 6;
		m_VuReflVX3Table.nMask			= 0x1F;
		m_VuReflVX3Table.pTable			= m_VuReflVX3;

		m_VuReflV[0x3C].subTable		= &m_VuReflVX0Table;
		m_VuReflV[0x3D].subTable		= &m_VuReflVX1Table;
		m_VuReflV[0x3E].subTable		= &m_VuReflVX2Table;
		m_VuReflV[0x3F].subTable		= &m_VuReflVX3Table;
	}
}

void CMA_VU::CUpper::GetInstructionMnemonic(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	INSTRUCTION Instr;
	Instr.pGetMnemonic	= SubTableMnemonic;
	Instr.pSubTable		= &m_ReflVTable;
	Instr.pGetMnemonic(&Instr, pCtx, nOpcode, sText, nCount);
}

void CMA_VU::CUpper::GetInstructionOperands(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	INSTRUCTION Instr;
	Instr.pGetOperands	= SubTableOperands;
	Instr.pSubTable		= &m_ReflVTable;
	Instr.pGetOperands(&Instr, pCtx, nAddress, nOpcode, sText, nCount);
}

MIPS_BRANCH_TYPE CMA_VU::CUpper::IsInstructionBranch(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	INSTRUCTION Instr;
	Instr.pIsBranch		= SubTableIsBranch;
	Instr.pSubTable		= &m_ReflVTable;
	return Instr.pIsBranch(&Instr, pCtx, nOpcode);
}

uint32 CMA_VU::CUpper::GetInstructionEffectiveAddress(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	INSTRUCTION Instr;
	Instr.pGetEffectiveAddress	= SubTableEffAddr;
	Instr.pSubTable				= &m_ReflVTable;
	return Instr.pGetEffectiveAddress(&Instr, pCtx, nAddress, nOpcode);
}

VUShared::OPERANDSET CMA_VU::CUpper::GetAffectedOperands(CMIPS* context, uint32 address, uint32 opcode)
{
	OPERANDSET result;
	memset(&result, 0, sizeof(OPERANDSET));

	VUINSTRUCTION instr;
	instr.pGetAffectedOperands	= SubTableAffectedOperands;
	instr.subTable				= &m_VuReflVTable;
	instr.pGetAffectedOperands(&instr, context, address, opcode, result);
	return result;
}
