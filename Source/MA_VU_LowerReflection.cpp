#include <string.h>
#include <stdio.h>
#include <boost/static_assert.hpp>
#include "MIPS.h"
#include "MA_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CMA_VU::CLower::ReflOpIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIS = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VI%i", nIS);
}

void CMA_VU::CLower::ReflOpIsOfs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8  nIS	= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16 nImm	= static_cast<uint16>((nOpcode >>  0) & 0x07FF);

	nAddress += 8;

	sprintf(sText, "VI%i, $%0.8X", nIS, nAddress + GetBranch(nImm));
}

void CMA_VU::CLower::ReflOpIt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT = static_cast<uint8>((nOpcode >> 16) & 0x001F);

	sprintf(sText, "VI%i", nIT);
}

void CMA_VU::CLower::ReflOpImm12(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint16	nImm	= static_cast<uint16>((nOpcode & 0x7FF) | (nOpcode & 0x00200000) >> 10);

	sprintf(sText, "0x%0.3X", nImm);
}

void CMA_VU::CLower::ReflOpItImm12(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8	nIT		= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint16	nImm	= static_cast<uint16>((nOpcode & 0x7FF) | (nOpcode & 0x00200000) >> 10);

	sprintf(sText, "VI%i, 0x%0.3X", nIT, nImm);
}

void CMA_VU::CLower::ReflOpItIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VI%i, VI%i", nIT, nIS);
}

void CMA_VU::CLower::ReflOpItFsf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFSF	= static_cast<uint8>((nOpcode >> 21) & 0x0003);

	sprintf(sText, "VI%i, VF%i%s", nIT, nFS, m_sBroadcast[nFSF]);
}

void CMA_VU::CLower::ReflOpOfs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint16 nImm	= static_cast<uint16>((nOpcode >>  0) & 0x07FF);

	nAddress += 8;

	sprintf(sText, "$%0.8X", nAddress + GetBranch(nImm));
}

void CMA_VU::CLower::ReflOpItOfs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint16 nImm = static_cast<uint16>((nOpcode >>  0) & 0x07FF);
	uint8  nIT  = static_cast<uint8> ((nOpcode >> 16) & 0x001F);

    nAddress += 8;

	sprintf(sText, "VI%i, $%0.8X", nIT, nAddress + GetBranch(nImm));
}

void CMA_VU::CLower::ReflOpItIsOfs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT	= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16 nImm	= static_cast<uint16>((nOpcode >>  0) & 0x07FF);

	nAddress += 8;

	sprintf(sText, "VI%i, VI%i, $%0.8X", nIT, nIS, nAddress + GetBranch(nImm));
}

void CMA_VU::CLower::ReflOpItIsImm15(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8	nIT		= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint8	nIS		= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16	nImm	= static_cast<uint16>((nOpcode & 0x7FF) | (nOpcode & 0x01E00000) >> 10);

	sprintf(sText, "VI%i, VI%i, $%0.4X", nIT, nIS, nImm);
}

void CMA_VU::CLower::ReflOpIdIsIt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nID = static_cast<uint8>((nOpcode >>  6) & 0x001F);

	sprintf(sText, "VI%i, VI%i, VI%i", nID, nIS, nIT);
}

void CMA_VU::CLower::ReflOpItIsDst(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VI%i, (VI%i)%s", nIT, nIS, m_sDestination[nDest]);
}

void CMA_VU::CLower::ReflOpItOfsIsDst(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8> ((nOpcode >> 21) & 0x000F);
	uint8 nIT	= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16 nImm	= static_cast<uint16>((nOpcode >>  0) & 0x07FF);
	if(nImm & 0x400) nImm |= 0xF800;

	sprintf(sText, "VI%i, $%0.4X(VI%i)%s", nIT, nImm, nIS, m_sDestination[nDest]);
}

void CMA_VU::CLower::ReflOpImm24(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint32 nImm = nOpcode & 0xFFFFFF;

	sprintf(sText, "$%0.6X", nImm);
}

void CMA_VU::CLower::ReflOpVi1Imm24(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint32 nImm = nOpcode & 0xFFFFFF;

	sprintf(sText, "VI1, $%0.6X", nImm);
}

void CMA_VU::CLower::ReflOpFtIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, VI%i", nFT, m_sDestination[nDest], nIS);
}

void CMA_VU::CLower::ReflOpFtDstIsInc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, (VI%i++)", nFT, m_sDestination[nDest], nIS);
}

void CMA_VU::CLower::ReflOpFtDstIsDec(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, (--VI%i)", nFT, m_sDestination[nDest], nIS);
}

void CMA_VU::CLower::ReflOpFsDstOfsIt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8  nDest	= static_cast<uint8> ((nOpcode >> 21) & 0x000F);
	uint8  nIT		= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint8  nFS		= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16 nImm		= static_cast<uint16>((nOpcode >>  0) & 0x07FF);
	if(nImm & 0x400) nImm |= 0xF800;

	sprintf(sText, "VF%i%s, $%0.4X(VI%i)", nFS, m_sDestination[nDest], nImm, nIT);
}

void CMA_VU::CLower::ReflOpFtDstOfsIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8  nDest	= static_cast<uint8> ((nOpcode >> 21) & 0x000F);
	uint8  nFT		= static_cast<uint8> ((nOpcode >> 16) & 0x001F);
	uint8  nIS		= static_cast<uint8> ((nOpcode >> 11) & 0x001F);
	uint16 nImm		= static_cast<uint16>((nOpcode >>  0) & 0x07FF);
	if(nImm & 0x400) nImm |= 0xF800;

	sprintf(sText, "VF%i%s, $%0.4X(VI%i)", nFT, m_sDestination[nDest], nImm, nIS);
}

void CMA_VU::CLower::ReflOpFtDstFsDst(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void CMA_VU::CLower::ReflOpPFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "P, VF%i", nFS);
}

void CMA_VU::CLower::ReflOpPFsf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFSF	= static_cast<uint8>((nOpcode >> 21) & 0x0003);

	sprintf(sText, "P, VF%i%s", nFS, m_sBroadcast[nFSF]);
}

void CMA_VU::CLower::ReflOpFtP(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);

	sprintf(sText, "VF%i%s, P", nFT, m_sDestination[nDest]);
}

uint32 CMA_VU::CLower::ReflEaOffset(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	uint16 nImm	= static_cast<uint16>((nOpcode >>  0) & 0x07FF);

	nAddress += 8;
	return (nAddress + GetBranch(nImm));
}

uint32 CMA_VU::CLower::ReflEaIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	uint8 nIS = static_cast<uint8>((nOpcode >> 11) & 0x001F);

	return pCtx->m_State.nCOP2VI[nIS] & 0xFFFF;
}

/////////////////////////////////////////////////
// Extended Reflection Stuff
/////////////////////////////////////////////////

void CMA_VU::CLower::ReflOpAffFtDstOfsIs(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nIS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.writeF = nFT;
	operandSet.readI0 = nIS;
}

void CMA_VU::CLower::ReflOpAffFsDstOfsIt(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nIT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.readF0 = nFS;
	operandSet.readI0 = nIT;
}

void CMA_VU::CLower::ReflOpAffFtDstFsDst(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.readF0 = nFS;
	operandSet.writeF = nFT;
}

void CMA_VU::CLower::ReflOpAffFsDstItInc(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nIT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.readI0 = nIT;
	operandSet.writeI = nIT;
	operandSet.readF0 = nFS;
}

void CMA_VU::CLower::ReflOpAffFtIs(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nIS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.readI0 = nIS;
	operandSet.writeF = nFT;
}

void CMA_VU::CLower::ReflOpAffFtDstIsInc(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8 )((nOpcode >> 16) & 0x001F);
	uint8 nIS		= (uint8 )((nOpcode >> 11) & 0x001F);

	operandSet.writeF = nFT;
	operandSet.readI0 = nIS;
	operandSet.writeI = nIS;
}

void CMA_VU::CLower::ReflOpAffItFsf(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nIT		= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	operandSet.writeI = nIT;
	operandSet.readF0 = nFS;
}

void CMA_VU::CLower::ReflOpAffPFs(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);
	operandSet.readF0 = nFS;
}

void CMA_VU::CLower::ReflOpAffPFsf(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);
	operandSet.readF0 = nFS;
}

INSTRUCTION CMA_VU::CLower::m_cReflGeneral[128] =
{
	//0x00
	{	"LQ",		NULL,			CopyMnemonic,		ReflOpFtDstOfsIs,	NULL,				NULL			},
	{	"SQ",		NULL,			CopyMnemonic,		ReflOpFsDstOfsIt,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ILW",		NULL,			CopyMnemonic,		ReflOpItOfsIsDst,	NULL,				NULL			},
	{	"ISW",		NULL,			CopyMnemonic,		ReflOpItOfsIsDst,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x08
	{	"IADDIU",	NULL,			CopyMnemonic,		ReflOpItIsImm15,	NULL,				NULL			},
	{	"ISUBIU",	NULL,			CopyMnemonic,		ReflOpItIsImm15,	NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x10
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FCSET",	NULL,			CopyMnemonic,		ReflOpImm24,		NULL,				NULL			},
	{	"FCAND",	NULL,			CopyMnemonic,		ReflOpVi1Imm24,		NULL,				NULL			},
	{	"FCOR",		NULL,			CopyMnemonic,		ReflOpVi1Imm24,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FSSET",	NULL,			CopyMnemonic,		ReflOpImm12,		NULL,				NULL			},
	{	"FSAND",	NULL,			CopyMnemonic,		ReflOpItImm12,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FMAND",	NULL,			CopyMnemonic,		ReflOpItIs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FCGET",	NULL,			CopyMnemonic,		ReflOpIt,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x20
	{	"B",		NULL,			CopyMnemonic,		ReflOpOfs,			IsBranch,			ReflEaOffset	},
	{	"BAL",		NULL,			CopyMnemonic,		ReflOpItOfs,		IsBranch,			ReflEaOffset	},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"JR",		NULL,			CopyMnemonic,		ReflOpIs,			IsBranch,			NULL			},
	{	"JALR",		NULL,			CopyMnemonic,		ReflOpItIs,			IsBranch,			NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x28
	{	"IBEQ",		NULL,			CopyMnemonic,		ReflOpItIsOfs,		IsBranch,			ReflEaOffset	},
	{	"IBNE",		NULL,			CopyMnemonic,		ReflOpItIsOfs,		IsBranch,			ReflEaOffset	},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"IBLTZ",	NULL,			CopyMnemonic,		ReflOpIsOfs,		IsBranch,			ReflEaOffset	},
	{	"IBGTZ",	NULL,			CopyMnemonic,		ReflOpIsOfs,		IsBranch,			ReflEaOffset	},
	{	"IBLEZ",	NULL,			CopyMnemonic,		ReflOpIsOfs,		IsBranch,			ReflEaOffset	},
	{	"IBGEZ",	NULL,			CopyMnemonic,		ReflOpIsOfs,		IsBranch,			ReflEaOffset	},
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
	//0x40
	{	"LowerOP",	NULL,			SubTableMnemonic,	SubTableOperands,	SubTableIsBranch,	SubTableEffAddr	},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x48
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x50
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x58
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x60
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x68
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x70
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x78
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CMA_VU::CLower::m_cReflV[64] =
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
	{	"IADD",		NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,				NULL			},
	{	"ISUB",		NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,				NULL			},
	{	"IADDI",	NULL,			CopyMnemonic,		ReflOpItIsImm5,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"IAND",		NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,				NULL			},
	{	"IOR",		NULL,			CopyMnemonic,		ReflOpIdIsIt,		NULL,				NULL			},
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

INSTRUCTION CMA_VU::CLower::m_cReflVX0[32] =
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
	{	"MOVE",		NULL,			CopyMnemonic,		ReflOpFtDstFsDst,	NULL,				NULL			},
	{	"LQI",		NULL,			CopyMnemonic,		ReflOpFtDstIsInc,	NULL,				NULL			},
	{	"DIV",		NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	"MTIR",		NULL,			CopyMnemonic,		ReflOpItFsf,		NULL,				NULL			},
	//0x10
	{	"RNEXT",	NULL,			CopyMnemonic,		ReflOpFtR,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	//0x18
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"MFP",		NULL,			CopyMnemonic,		ReflOpFtP,			NULL,				NULL			},
	{	"XTOP",		NULL,			CopyMnemonic,		ReflOpIt,			NULL,				NULL			},
	{	"XGKICK",	NULL,			CopyMnemonic,		ReflOpIs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ESQRT",	NULL,			CopyMnemonic,		ReflOpPFsf,			NULL,				NULL			},
	{	"ESIN",	    NULL,			CopyMnemonic,		ReflOpPFsf,			NULL,				NULL			},
};

INSTRUCTION CMA_VU::CLower::m_cReflVX1[32] =
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
	{	"MR32",		NULL,			CopyMnemonic,		ReflOpFtDstFsDst,	NULL,				NULL			},
	{	"SQI",		NULL,			CopyMnemonic,		ReflOpFsDstItInc,	NULL,				NULL			},
	{	"SQRT",		NULL,			CopyMnemonic,		ReflOpQFtf,			NULL,				NULL			},
	{	"MFIR",		NULL,			CopyMnemonic,		ReflOpFtIs,			NULL,				NULL			},
	//0x10
	{	"RGET",		NULL,			CopyMnemonic,		ReflOpFtR,			NULL,				NULL			},
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
	{	"XITOP",	NULL,			CopyMnemonic,		ReflOpIt,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CMA_VU::CLower::m_cReflVX2[32] =
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
	{	"LQD",		NULL,			CopyMnemonic,		ReflOpFtDstIsDec,	NULL,				NULL			},
	{	"RSQRT",	NULL,			CopyMnemonic,		ReflOpQFsfFtf,		NULL,				NULL			},
	{	"ILWR",		NULL,			CopyMnemonic,		ReflOpItIsDst,		NULL,				NULL			},
	//0x10
	{	"RINIT",	NULL,			CopyMnemonic,		ReflOpRFsf,			NULL,				NULL			},
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
	{	"ELENG",	NULL,			CopyMnemonic,		ReflOpPFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"ERCPR",	NULL,			CopyMnemonic,		ReflOpPFsf,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

INSTRUCTION CMA_VU::CLower::m_cReflVX3[32] =
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
	{	"WAITQ",	NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	"ISWR",		NULL,			CopyMnemonic,		ReflOpItIsDst,		NULL,				NULL			},
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
	{	"ERLENG",	NULL,			CopyMnemonic,		ReflOpPFs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"WAITP",	NULL,			CopyMnemonic,		NULL,				NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflGeneral[128] =
{
	//0x00
	{	"LQ",		NULL,			ReflOpAffFtDstOfsIs	},
	{	"SQ",		NULL,			ReflOpAffFsDstOfsIt	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ILW",		NULL,			NULL				},
	{	"ISW",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	"IADDIU",	NULL,			NULL				},
	{	"ISUBIU",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x10
	{	NULL,		NULL,			NULL				},
	{	"FCSET",	NULL,			NULL				},
	{	"FCAND",	NULL,			NULL				},
	{	"FCOR",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"FSAND",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"FMAND",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"FCGET",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x20
	{	"B",		NULL,			NULL				},
	{	"BAL",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"JR",		NULL,			NULL				},
	{	"JALR",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x28
	{	"IBEQ",		NULL,			NULL				},
	{	"IBNE",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"IBLTZ",	NULL,			NULL				},
	{	"IBGTZ",	NULL,			NULL				},
	{	"IBLEZ",	NULL,			NULL				},
	{	"IBGEZ",	NULL,			NULL				},
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
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x40
	{	"LowerOP",	NULL,			SubTableAffectedOperands	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x48
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x50
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x58
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x60
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x68
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x70
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x78
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflV[64] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
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
	//0x20
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x28
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x30
	{	"IADD",		NULL,			NULL				},
	{	"ISUB",		NULL,			NULL				},
	{	"IADDI",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"IAND",		NULL,			NULL				},
	{	"IOR",		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x38
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"Vx0",		NULL,			SubTableAffectedOperands	},
	{	"Vx1",		NULL,			SubTableAffectedOperands	},
	{	"Vx2",		NULL,			SubTableAffectedOperands	},
	{	"Vx3",		NULL,			SubTableAffectedOperands	},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflVX0[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MOVE",		NULL,			ReflOpAffFtDstFsDst	},
	{	"LQI",		NULL,			ReflOpAffFtDstIsInc	},
	{	"DIV",		NULL,			ReflOpAffQFsfFtf	},
	{	"MTIR",		NULL,			ReflOpAffItFsf		},
	//0x10
	{	"RNEXT",	NULL,			ReflOpAffFtR		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x18
	{	NULL,		NULL,			NULL				},
	{	"MFP",		NULL,			NULL				},
	{	"XTOP",		NULL,			NULL				},
	{	"XGKICK",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ESIN",		NULL,			ReflOpAffPFsf		},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflVX1[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"MR32",		NULL,			ReflOpAffFtDstFsDst	},
	{	"SQI",		NULL,			ReflOpAffFsDstItInc	},
	{	NULL,		NULL,			NULL				},
	{	"MFIR",		NULL,			ReflOpAffFtIs		},
	//0x10
	{	"RGET",		NULL,			ReflOpAffFtR		},
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
	{	"XITOP",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflVX2[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"RSQRT",	NULL,			ReflOpAffQFsfFtf	},
	{	"ILWR",		NULL,			NULL				},
	//0x10
	{	"RINIT",	NULL,			ReflOpAffRFsf		},
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
	{	"ERCPR",	NULL,			ReflOpAffPFsf		},
	{	NULL,		NULL,			NULL				},
};

VUINSTRUCTION CMA_VU::CLower::m_cVuReflVX3[32] =
{
	//0x00
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"WAITQ",	NULL,			ReflOpAffQ			},
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
	{	"ERLENG",	NULL,			ReflOpAffPFs		},
	{	NULL,		NULL,			NULL				},
	{	"WAITP",	NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
};

void CMA_VU::CLower::SetupReflectionTables()
{
	static_assert(sizeof(m_ReflGeneral)		== sizeof(m_cReflGeneral),		"Array sizes don't match");
	static_assert(sizeof(m_ReflV)			== sizeof(m_cReflV),			"Array sizes don't match");
	static_assert(sizeof(m_ReflVX0)			== sizeof(m_cReflVX0),			"Array sizes don't match");
	static_assert(sizeof(m_ReflVX1)			== sizeof(m_cReflVX1),			"Array sizes don't match");
	static_assert(sizeof(m_ReflVX2)			== sizeof(m_cReflVX2),			"Array sizes don't match");
	static_assert(sizeof(m_ReflVX3)			== sizeof(m_cReflVX3),			"Array sizes don't match");

	static_assert(sizeof(m_VuReflGeneral)	== sizeof(m_cVuReflGeneral),	"Array sizes don't match");
	static_assert(sizeof(m_VuReflV)			== sizeof(m_cVuReflV),			"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX0)		== sizeof(m_cVuReflVX0),		"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX1)		== sizeof(m_cVuReflVX1),		"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX2)		== sizeof(m_cVuReflVX2),		"Array sizes don't match");
	static_assert(sizeof(m_VuReflVX3)		== sizeof(m_cVuReflVX3),		"Array sizes don't match");

	memcpy(m_ReflGeneral,	m_cReflGeneral, sizeof(m_cReflGeneral));
	memcpy(m_ReflV,			m_cReflV,		sizeof(m_cReflV));
	memcpy(m_ReflVX0,		m_cReflVX0,		sizeof(m_cReflVX0));
	memcpy(m_ReflVX1,		m_cReflVX1,		sizeof(m_cReflVX1));
	memcpy(m_ReflVX2,		m_cReflVX2,		sizeof(m_cReflVX2));
	memcpy(m_ReflVX3,		m_cReflVX3,		sizeof(m_cReflVX3));

	memcpy(m_VuReflGeneral,	m_cVuReflGeneral,	sizeof(m_cVuReflGeneral));
	memcpy(m_VuReflV,		m_cVuReflV,			sizeof(m_cVuReflV));
	memcpy(m_VuReflVX0,		m_cVuReflVX0,		sizeof(m_cVuReflVX0));
	memcpy(m_VuReflVX1,		m_cVuReflVX1,		sizeof(m_cVuReflVX1));
	memcpy(m_VuReflVX2,		m_cVuReflVX2,		sizeof(m_cVuReflVX2));
	memcpy(m_VuReflVX3,		m_cVuReflVX3,		sizeof(m_cVuReflVX3));

	{
		m_ReflGeneralTable.nShift		= 25;
		m_ReflGeneralTable.nMask		= 0x7F;
		m_ReflGeneralTable.pTable		= m_ReflGeneral;

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

		m_ReflGeneral[0x40].pSubTable	= &m_ReflVTable;

		m_ReflV[0x3C].pSubTable			= &m_ReflVX0Table;
		m_ReflV[0x3D].pSubTable			= &m_ReflVX1Table;
		m_ReflV[0x3E].pSubTable			= &m_ReflVX2Table;
		m_ReflV[0x3F].pSubTable			= &m_ReflVX3Table;
	}

	{
		m_VuReflGeneralTable.nShift			= 25;
		m_VuReflGeneralTable.nMask			= 0x7F;
		m_VuReflGeneralTable.pTable			= m_VuReflGeneral;

		m_VuReflVTable.nShift				= 0;
		m_VuReflVTable.nMask				= 0x3F;
		m_VuReflVTable.pTable				= m_VuReflV;

		m_VuReflVX0Table.nShift				= 6;
		m_VuReflVX0Table.nMask				= 0x1F;
		m_VuReflVX0Table.pTable				= m_VuReflVX0;

		m_VuReflVX1Table.nShift				= 6;
		m_VuReflVX1Table.nMask				= 0x1F;
		m_VuReflVX1Table.pTable				= m_VuReflVX1;

		m_VuReflVX2Table.nShift				= 6;
		m_VuReflVX2Table.nMask				= 0x1F;
		m_VuReflVX2Table.pTable				= m_VuReflVX2;

		m_VuReflVX3Table.nShift				= 6;
		m_VuReflVX3Table.nMask				= 0x1F;
		m_VuReflVX3Table.pTable				= m_VuReflVX3;

		m_VuReflGeneral[0x40].subTable		= &m_VuReflVTable;

		m_VuReflV[0x3C].subTable			= &m_VuReflVX0Table;
		m_VuReflV[0x3D].subTable			= &m_VuReflVX1Table;
		m_VuReflV[0x3E].subTable			= &m_VuReflVX2Table;
		m_VuReflV[0x3F].subTable			= &m_VuReflVX3Table;
	}
}

void CMA_VU::CLower::GetInstructionMnemonic(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	if(pCtx->m_pMemoryMap->GetInstruction(nAddress + 4) & 0x80000000)
	{
		strncpy(sText, "LOI", nCount);
		return;
	}

	if(nOpcode == 0x8000033C)
	{
		strncpy(sText, "NOP", nCount);
		return;
	}

	INSTRUCTION Instr;
	Instr.pGetMnemonic	= SubTableMnemonic;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetMnemonic(&Instr, pCtx, nOpcode, sText, nCount);
}

void CMA_VU::CLower::GetInstructionOperands(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	if(pCtx->m_pMemoryMap->GetInstruction(nAddress + 4) & 0x80000000)
	{
		sprintf(sText, "$%0.8X", nOpcode);
		return;
	}

	if(nOpcode == 0x8000033C)
	{
		strncpy(sText, "", nCount);
		return;
	}

	INSTRUCTION Instr;
	Instr.pGetOperands	= SubTableOperands;
	Instr.pSubTable		= &m_ReflGeneralTable;
	Instr.pGetOperands(&Instr, pCtx, nAddress, nOpcode, sText, nCount);
}

MIPS_BRANCH_TYPE CMA_VU::CLower::IsInstructionBranch(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	if(nOpcode == 0x8000033C)
	{
		return MIPS_BRANCH_NONE;
	}

	INSTRUCTION Instr;
	Instr.pIsBranch		= SubTableIsBranch;
	Instr.pSubTable		= &m_ReflGeneralTable;
	return Instr.pIsBranch(&Instr, pCtx, nOpcode);
}

uint32 CMA_VU::CLower::GetInstructionEffectiveAddress(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	if(nOpcode == 0x8000033C)
	{
		return 0;
	}

	INSTRUCTION Instr;
	Instr.pGetEffectiveAddress	= SubTableEffAddr;
	Instr.pSubTable				= &m_ReflGeneralTable;
	return Instr.pGetEffectiveAddress(&Instr, pCtx, nAddress, nOpcode);
}

VUShared::OPERANDSET CMA_VU::CLower::GetAffectedOperands(CMIPS* context, uint32 address, uint32 opcode)
{
	OPERANDSET result;
	memset(&result, 0, sizeof(OPERANDSET));

	VUINSTRUCTION instr;
	instr.pGetAffectedOperands	= SubTableAffectedOperands;
	instr.subTable				= &m_VuReflGeneralTable;
	instr.pGetAffectedOperands(&instr, context, address, opcode, result);
	return result;
}
