#include "MIPS.h"
#include "MA_ALLEGREX.h"

using namespace MIPSReflection;

void CMA_ALLEGREX::ReflOpIns(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 rs	= static_cast<uint8>((nOpcode >> 21) & 0x001F);
	uint8 rt	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 size	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 pos	= static_cast<uint8>((nOpcode >>  6) & 0x001F);

	size = size - pos + 1;

	sprintf(sText, "%s, %s, %i, %i", CMIPS::m_sGPRName[rt], CMIPS::m_sGPRName[rs], pos, size);
}

INSTRUCTION CMA_ALLEGREX::m_cReflSpecial3[64] =
{
	//0x00
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"INS",		NULL,			CopyMnemonic,		ReflOpIns,			NULL,				NULL			},
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
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x38
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

void CMA_ALLEGREX::SetupReflectionTables()
{
    BOOST_STATIC_ASSERT(sizeof(m_ReflSpecial3) == sizeof(m_cReflSpecial3));

	memcpy(m_ReflSpecial3,	m_cReflSpecial3,	sizeof(m_cReflSpecial3));

	m_ReflSpecial3Table.pTable					= m_ReflSpecial3;
	m_ReflSpecial3Table.nShift					= 0;
	m_ReflSpecial3Table.nMask					= 0x3F;

	//Fix MIPSIV tables
	m_ReflGeneral[0x1F].sMnemonic				= "SPECIAL3";
	m_ReflGeneral[0x1F].pSubTable				= &m_ReflSpecial3Table;
	m_ReflGeneral[0x1F].pGetMnemonic			= SubTableMnemonic;
	m_ReflGeneral[0x1F].pGetOperands			= SubTableOperands;
	m_ReflGeneral[0x1F].pIsBranch				= SubTableIsBranch;
	m_ReflGeneral[0x1F].pGetEffectiveAddress	= SubTableEffAddr;
}
