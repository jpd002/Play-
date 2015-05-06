#include <string.h>
#include <stdio.h>
#include "../MIPS.h"
#include "MA_VU.h"
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

void CMA_VU::CLower::ReflOpIs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 is = static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VI%i", is);
}

void CMA_VU::CLower::ReflOpIsOfs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8  is	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);

	address += 8;

	sprintf(text, "VI%i, $%0.8X", is, address + GetBranch(imm));
}

void CMA_VU::CLower::ReflOpIt(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 it = static_cast<uint8>((opcode >> 16) & 0x001F);

	sprintf(text, "VI%i", it);
}

void CMA_VU::CLower::ReflOpImm12(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint16	imm	= static_cast<uint16>((opcode & 0x7FF) | (opcode & 0x00200000) >> 10);

	sprintf(text, "0x%0.3X", imm);
}

void CMA_VU::CLower::ReflOpItImm12(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8	it	= static_cast<uint8>((opcode >> 16) & 0x001F);
	uint16	imm	= static_cast<uint16>((opcode & 0x7FF) | (opcode & 0x00200000) >> 10);

	sprintf(text, "VI%i, 0x%0.3X", it, imm);
}

void CMA_VU::CLower::ReflOpItIs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 it	= static_cast<uint8>((opcode >> 16) & 0x001F);
	uint8 is	= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VI%i, VI%i", it, is);
}

void CMA_VU::CLower::ReflOpOfs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);

	address += 8;

	sprintf(text, "$%0.8X", address + GetBranch(imm));
}

void CMA_VU::CLower::ReflOpItOfs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint16 imm = static_cast<uint16>((opcode >>  0) & 0x07FF);
	uint8  it  = static_cast<uint8> ((opcode >> 16) & 0x001F);

	address += 8;

	sprintf(text, "VI%i, $%0.8X", it, address + GetBranch(imm));
}

void CMA_VU::CLower::ReflOpItIsOfs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 it	= static_cast<uint8> ((opcode >> 16) & 0x001F);
	uint8 is	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);

	address += 8;

	sprintf(text, "VI%i, VI%i, $%0.8X", it, is, address + GetBranch(imm));
}

void CMA_VU::CLower::ReflOpItIsImm15(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8	it	= static_cast<uint8> ((opcode >> 16) & 0x001F);
	uint8	is	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16	imm	= static_cast<uint16>((opcode & 0x7FF) | (opcode & 0x01E00000) >> 10);

	sprintf(text, "VI%i, VI%i, $%0.4X", it, is, imm);
}

void CMA_VU::CLower::ReflOpItOfsIsDst(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 dest	= static_cast<uint8> ((opcode >> 21) & 0x000F);
	uint8 it	= static_cast<uint8> ((opcode >> 16) & 0x001F);
	uint8 is	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);
	if(imm & 0x400) imm |= 0xF800;

	sprintf(text, "VI%i, $%0.4X(VI%i)%s", it, imm, is, m_sDestination[dest]);
}

void CMA_VU::CLower::ReflOpImm24(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint32 imm = opcode & 0xFFFFFF;

	sprintf(text, "$%0.6X", imm);
}

void CMA_VU::CLower::ReflOpVi1Imm24(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint32 imm = opcode & 0xFFFFFF;

	sprintf(text, "VI1, $%0.6X", imm);
}

void CMA_VU::CLower::ReflOpFtDstIsDec(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 dest	= static_cast<uint8>((opcode >> 21) & 0x000F);
	uint8 ft	= static_cast<uint8>((opcode >> 16) & 0x001F);
	uint8 is	= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VF%i%s, (--VI%i)", ft, m_sDestination[dest], is);
}

void CMA_VU::CLower::ReflOpFsDstOfsIt(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8  dest	= static_cast<uint8> ((opcode >> 21) & 0x000F);
	uint8  it	= static_cast<uint8> ((opcode >> 16) & 0x001F);
	uint8  fs	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);
	if(imm & 0x400) imm |= 0xF800;

	sprintf(text, "VF%i%s, $%0.4X(VI%i)", fs, m_sDestination[dest], imm, it);
}

void CMA_VU::CLower::ReflOpFtDstOfsIs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8  dest	= static_cast<uint8> ((opcode >> 21) & 0x000F);
	uint8  ft	= static_cast<uint8> ((opcode >> 16) & 0x001F);
	uint8  is	= static_cast<uint8> ((opcode >> 11) & 0x001F);
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);
	if(imm & 0x400) imm |= 0xF800;

	sprintf(text, "VF%i%s, $%0.4X(VI%i)", ft, m_sDestination[dest], imm, is);
}

void CMA_VU::CLower::ReflOpFtDstFsDst(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 dest	= static_cast<uint8>((opcode >> 21) & 0x000F);
	uint8 ft	= static_cast<uint8>((opcode >> 16) & 0x001F);
	uint8 fs	= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VF%i%s, VF%i%s", ft, m_sDestination[dest], fs, m_sDestination[dest]);
}

void CMA_VU::CLower::ReflOpPFs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 fs	= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "P, VF%i", fs);
}

void CMA_VU::CLower::ReflOpPFsf(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 fs	= static_cast<uint8>((opcode >> 11) & 0x001F);
	uint8 fsf	= static_cast<uint8>((opcode >> 21) & 0x0003);

	sprintf(text, "P, VF%i%s", fs, m_sBroadcast[fsf]);
}

void CMA_VU::CLower::ReflOpFtP(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 dest	= static_cast<uint8>((opcode >> 21) & 0x000F);
	uint8 ft	= static_cast<uint8>((opcode >> 16) & 0x001F);

	sprintf(text, "VF%i%s, P", ft, m_sDestination[dest]);
}

uint32 CMA_VU::CLower::ReflEaOffset(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode)
{
	uint16 imm	= static_cast<uint16>((opcode >>  0) & 0x07FF);

	address += 8;
	return (address + GetBranch(imm));
}

uint32 CMA_VU::CLower::ReflEaIs(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode)
{
	uint8 is = static_cast<uint8>((opcode >> 11) & 0x001F);

	return context->m_State.nCOP2VI[is] & 0xFFFF;
}

/////////////////////////////////////////////////
// Extended Reflection Stuff
/////////////////////////////////////////////////

void CMA_VU::CLower::ReflOpAffRdIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.readI0 = is;
}

void CMA_VU::CLower::ReflOpAffRdItFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.readI0 = it;
	operandSet.readF0 = fs;
}

void CMA_VU::CLower::ReflOpAffRdItIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.readI0 = it;
	operandSet.readI1 = is;
}

void CMA_VU::CLower::ReflOpAffRdP(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	//TODO: Read P
}

void CMA_VU::CLower::ReflOpAffWrFtRdFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeF = ft;
	operandSet.readF0 = fs;
}

void CMA_VU::CLower::ReflOpAffWrFtIsRdIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeF = ft;
	operandSet.readI0 = is;
	operandSet.writeI = is;
}

void CMA_VU::CLower::ReflOpAffWrFtRdIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeF = ft;
	operandSet.readI0 = is;
}

void CMA_VU::CLower::ReflOpAffWrFtRdP(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);

	//TODO: Read P
	operandSet.writeF = ft;
}

void CMA_VU::CLower::ReflOpAffWrIdRdItIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);
	auto id = static_cast<uint8>((opcode >>  6) & 0x001F);

	operandSet.readI0 = it;
	operandSet.readI1 = is;
	operandSet.writeI = id;
}

void CMA_VU::CLower::ReflOpAffWrIt(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);

	operandSet.writeI = it;
}

void CMA_VU::CLower::ReflOpAffWrItRdFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeI = it;
	operandSet.readF0 = fs;
}

void CMA_VU::CLower::ReflOpAffWrItRdIs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeI = it;
	operandSet.readI0 = is;
}

void CMA_VU::CLower::ReflOpAffWrItRdItFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto it = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	operandSet.writeI = it;
	operandSet.readI0 = it;
	operandSet.readF0 = fs;
}

void CMA_VU::CLower::ReflOpAffWrPRdFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	//TODO: Write P
	operandSet.readF0 = fs;
}

void CMA_VU::CLower::ReflOpAffWrVi1(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	operandSet.writeI = 1;
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
	{	"FCEQ",		NULL,			CopyMnemonic,		ReflOpVi1Imm24,		NULL,				NULL			},
	{	"FCSET",	NULL,			CopyMnemonic,		ReflOpImm24,		NULL,				NULL			},
	{	"FCAND",	NULL,			CopyMnemonic,		ReflOpVi1Imm24,		NULL,				NULL			},
	{	"FCOR",		NULL,			CopyMnemonic,		ReflOpVi1Imm24,		NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FSSET",	NULL,			CopyMnemonic,		ReflOpImm12,		NULL,				NULL			},
	{	"FSAND",	NULL,			CopyMnemonic,		ReflOpItImm12,		NULL,				NULL			},
	{	"FSOR",		NULL,			CopyMnemonic,		ReflOpItImm12,		NULL,				NULL			},
	//0x18
	{	"FMEQ",		NULL,			CopyMnemonic,		ReflOpItIs,			NULL,				NULL			},
	{	NULL,		NULL,			NULL,				NULL,				NULL,				NULL			},
	{	"FMAND",	NULL,			CopyMnemonic,		ReflOpItIs,			NULL,				NULL			},
	{	"FMOR",		NULL,			CopyMnemonic,		ReflOpItIs,			NULL,				NULL			},
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
	{	"ESIN",		NULL,			CopyMnemonic,		ReflOpPFsf,			NULL,				NULL			},
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
	{	"ERSQRT",	NULL,			CopyMnemonic,		ReflOpPFsf,			NULL,				NULL			},
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
	{	"ESUM",		NULL,			CopyMnemonic,		ReflOpPFs,			NULL,				NULL			},
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
	{	"SQD",		NULL,			CopyMnemonic,		ReflOpFsDstItDec,	NULL,				NULL			},
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
	{	"LQ",		NULL,			ReflOpAffWrFtRdIs	},
	{	"SQ",		NULL,			ReflOpAffRdItFs		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ILW",		NULL,			ReflOpAffWrItRdIs	},
	{	"ISW",		NULL,			ReflOpAffRdItIs		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x08
	{	"IADDIU",	NULL,			ReflOpAffWrItRdIs	},
	{	"ISUBIU",	NULL,			ReflOpAffWrItRdIs	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x10
	{	"FCEQ",		NULL,			ReflOpAffWrVi1		},
	{	"FCSET",	NULL,			ReflOpAffNone		},
	{	"FCAND",	NULL,			ReflOpAffWrVi1		},
	{	"FCOR",		NULL,			ReflOpAffWrVi1		},
	{	NULL,		NULL,			NULL				},
	{	"FSSET",	NULL,			ReflOpAffNone		},
	{	"FSAND",	NULL,			ReflOpAffWrIt		},
	{	"FSOR",		NULL,			ReflOpAffWrIt		},
	//0x18
	{	"FMEQ",		NULL,			ReflOpAffWrItRdIs	},
	{	NULL,		NULL,			NULL				},
	{	"FMAND",	NULL,			ReflOpAffWrItRdIs	},
	{	"FMOR",		NULL,			ReflOpAffWrItRdIs	},
	{	"FCGET",	NULL,			ReflOpAffWrIt		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x20
	{	"B",		NULL,			ReflOpAffNone		},
	{	"BAL",		NULL,			ReflOpAffWrIt		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"JR",		NULL,			ReflOpAffRdIs		},
	{	"JALR",		NULL,			ReflOpAffWrItRdIs	},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	//0x28
	{	"IBEQ",		NULL,			ReflOpAffRdItIs		},
	{	"IBNE",		NULL,			ReflOpAffRdItIs		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"IBLTZ",	NULL,			ReflOpAffRdIs		},
	{	"IBGTZ",	NULL,			ReflOpAffRdIs		},
	{	"IBLEZ",	NULL,			ReflOpAffRdIs		},
	{	"IBGEZ",	NULL,			ReflOpAffRdIs		},
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
	{	"IADD",		NULL,			ReflOpAffWrIdRdItIs	},
	{	"ISUB",		NULL,			ReflOpAffWrIdRdItIs	},
	{	"IADDI",	NULL,			ReflOpAffWrItRdIs	},
	{	NULL,		NULL,			NULL				},
	{	"IAND",		NULL,			ReflOpAffWrIdRdItIs	},
	{	"IOR",		NULL,			ReflOpAffWrIdRdItIs	},
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
	{	"MOVE",		NULL,			ReflOpAffWrFtRdFs	},
	{	"LQI",		NULL,			ReflOpAffWrFtIsRdIs	},
	{	"DIV",		NULL,			ReflOpAffWrQRdFtFs	},
	{	"MTIR",		NULL,			ReflOpAffWrItRdFs	},
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
	{	"MFP",		NULL,			ReflOpAffWrFtRdP	},
	{	"XTOP",		NULL,			ReflOpAffWrIt		},
	{	"XGKICK",	NULL,			ReflOpAffRdIs		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ESQRT",	NULL,			ReflOpAffWrPRdFs	},
	{	"ESIN",		NULL,			ReflOpAffWrPRdFs	},
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
	{	"MR32",		NULL,			ReflOpAffWrFtRdFs	},
	{	"SQI",		NULL,			ReflOpAffWrItRdItFs	},
	{	"SQRT",		NULL,			ReflOpAffWrQRdFt	},
	{	"MFIR",		NULL,			ReflOpAffWrFtRdIs	},
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
	{	"XITOP",	NULL,			ReflOpAffWrIt		},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	NULL,		NULL,			NULL				},
	{	"ERSQRT",	NULL,			NULL				},
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
	{	"LQD",		NULL,			ReflOpAffWrFtIsRdIs	},
	{	"RSQRT",	NULL,			ReflOpAffWrQRdFtFs	},
	{	"ILWR",		NULL,			ReflOpAffWrItRdIs	},
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
	{	"ELENG",	NULL,			ReflOpAffWrPRdFs	},
	{	"ESUM",		NULL,			NULL				},
	{	"ERCPR",	NULL,			ReflOpAffWrPRdFs	},
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
	{	"SQD",		NULL,			ReflOpAffWrItRdItFs	},
	{	"WAITQ",	NULL,			ReflOpAffQ			},
	{	"ISWR",		NULL,			ReflOpAffRdItIs		},
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
	{	"ERLENG",	NULL,			ReflOpAffWrPRdFs	},
	{	NULL,		NULL,			NULL				},
	{	"WAITP",	NULL,			ReflOpAffRdP		},
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

	VerifyVuReflectionTable(m_cReflGeneral,	m_cVuReflGeneral,	sizeof(m_cReflGeneral)	/ sizeof(m_cReflGeneral[0]));
	VerifyVuReflectionTable(m_cReflV,		m_cVuReflV,			sizeof(m_cReflV)		/ sizeof(m_cReflV[0]));
	VerifyVuReflectionTable(m_cReflVX0,		m_cVuReflVX0,		sizeof(m_cReflVX0)		/ sizeof(m_cReflVX0[0]));
	VerifyVuReflectionTable(m_cReflVX1,		m_cVuReflVX1,		sizeof(m_cReflVX1)		/ sizeof(m_cReflVX1[0]));
	VerifyVuReflectionTable(m_cReflVX2,		m_cVuReflVX2,		sizeof(m_cReflVX2)		/ sizeof(m_cReflVX2[0]));
	VerifyVuReflectionTable(m_cReflVX3,		m_cVuReflVX3,		sizeof(m_cReflVX3)		/ sizeof(m_cReflVX3[0]));

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

void CMA_VU::CLower::GetInstructionMnemonic(CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	if(IsLOI(context, address))
	{
		strncpy(text, "LOI", count);
		return;
	}

	if(opcode == OPCODE_NOP)
	{
		strncpy(text, "NOP", count);
		return;
	}

	INSTRUCTION instr;
	instr.pGetMnemonic	= SubTableMnemonic;
	instr.pSubTable		= &m_ReflGeneralTable;
	instr.pGetMnemonic(&instr, context, opcode, text, count);
}

void CMA_VU::CLower::GetInstructionOperands(CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	if(IsLOI(context, address))
	{
		sprintf(text, "$%0.8X", opcode);
		return;
	}

	if(opcode == OPCODE_NOP)
	{
		strncpy(text, "", count);
		return;
	}

	INSTRUCTION instr;
	instr.pGetOperands	= SubTableOperands;
	instr.pSubTable		= &m_ReflGeneralTable;
	instr.pGetOperands(&instr, context, address, opcode, text, count);
}

MIPS_BRANCH_TYPE CMA_VU::CLower::IsInstructionBranch(CMIPS* context, uint32 address, uint32 opcode)
{
	if(IsLOI(context, address))
	{
		return MIPS_BRANCH_NONE;
	}

	if(opcode == OPCODE_NOP)
	{
		return MIPS_BRANCH_NONE;
	}

	INSTRUCTION instr;
	instr.pIsBranch		= SubTableIsBranch;
	instr.pSubTable		= &m_ReflGeneralTable;
	return instr.pIsBranch(&instr, context, opcode);
}

uint32 CMA_VU::CLower::GetInstructionEffectiveAddress(CMIPS* context, uint32 address, uint32 opcode)
{
	if(IsLOI(context, address))
	{
		return 0;
	}

	if(opcode == OPCODE_NOP)
	{
		return 0;
	}

	INSTRUCTION instr;
	instr.pGetEffectiveAddress	= SubTableEffAddr;
	instr.pSubTable				= &m_ReflGeneralTable;
	return instr.pGetEffectiveAddress(&instr, context, address, opcode);
}

VUShared::OPERANDSET CMA_VU::CLower::GetAffectedOperands(CMIPS* context, uint32 address, uint32 opcode)
{
	OPERANDSET result;
	memset(&result, 0, sizeof(OPERANDSET));

	if(IsLOI(context, address))
	{
		return result;
	}

	if(opcode == OPCODE_NOP)
	{
		return result;
	}

	VUINSTRUCTION instr;
	instr.pGetAffectedOperands	= SubTableAffectedOperands;
	instr.subTable				= &m_VuReflGeneralTable;
	instr.pGetAffectedOperands(&instr, context, address, opcode, result);
	return result;
}
