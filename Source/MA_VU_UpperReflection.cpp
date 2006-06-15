#include <string.h>
#include <stdio.h>
#include "MA_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CMA_VU::CUpper::ReflOpFtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

INSTRUCTION CMA_VU::CUpper::m_cReflV[64] =
{
	//0x00
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"SUB",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	//0x08
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	"MADD",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	"MAX",		NULL,			CopyMnemonic,		ReflOpFdFsFtBc,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ADD",		NULL,			CopyMnemonic,		ReflOpFdFsI,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ITOF0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"FTOI0",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FTOI4",	NULL,			CopyMnemonic,		ReflOpFtFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MULA",		NULL,			CopyMnemonic,		ReflOpAccFsI,		NULL,				NULL			},
	//0x08
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MADDA",	NULL,			CopyMnemonic,		ReflOpAccFsFtBc,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
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

void CMA_VU::CUpper::SetupReflectionTables()
{
	memcpy(m_ReflV,		m_cReflV,	sizeof(INSTRUCTION) * 64);
	memcpy(m_ReflVX0,	m_cReflVX0, sizeof(INSTRUCTION) * 32);
	memcpy(m_ReflVX1,	m_cReflVX1, sizeof(INSTRUCTION) * 32);
	memcpy(m_ReflVX2,	m_cReflVX2,	sizeof(INSTRUCTION) * 32);
	memcpy(m_ReflVX3,	m_cReflVX3, sizeof(INSTRUCTION) * 32);

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

bool CMA_VU::CUpper::IsInstructionBranch(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
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
