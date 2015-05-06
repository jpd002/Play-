#include <stdio.h>
#include "VUShared.h"

using namespace MIPSReflection;
using namespace VUShared;

const char* VUShared::m_sBroadcast[4] =
{
	"x",
	"y",
	"z",
	"w",
};

const char* VUShared::m_sDestination[16] =
{
	"",			//0000
	"w",		//000w
	"z",		//00z0
	"zw",		//00zw
	"y",		//0y00
	"yw",		//0y0w
	"yz",		//0yz0
	"yzw",		//0yzw
	"x",		//x000
	"xw",		//x00w
	"xz",		//x0z0
	"xzw",		//x0zw
	"xy",		//xy00
	"xyw",		//xy0w
	"xyz",		//xyz0
	"xyzw",		//xyzw
};

int32 VUShared::GetImm11Offset(uint16 nImm11)
{
	if(nImm11 & 0x400)
	{
		return -(0x800 - nImm11);
	}
	else
	{
		return (nImm11 & 0x3FF);
	}
}

int32 VUShared::GetBranch(uint16 nImm11)
{
	return GetImm11Offset(nImm11) * 8;
}

void VUShared::VerifyVuReflectionTable(MIPSReflection::INSTRUCTION* refl, VUShared::VUINSTRUCTION* vuRefl, size_t tableSize)
{
	for(unsigned int i = 0; i < tableSize; i++)
	{
		const char* reflMnem = refl[i].sMnemonic;
		const char* reflVuMnem = vuRefl[i].name;
		if((reflMnem == nullptr) || (reflVuMnem == nullptr))
		{
			assert(reflMnem == reflVuMnem);
		}
		else
		{
			assert(!strcmp(reflMnem, reflVuMnem));
		}
	}
}

void VUShared::ReflOpFdFsI(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD	= (uint8)((nOpcode >>  6) & 0x001F);

	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, I", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsQ(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD	= (uint8)((nOpcode >>  6) & 0x001F);

	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, Q", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD	= (uint8)((nOpcode >>  6) & 0x001F);

	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, VF%i%s", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsFtBc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD	= (uint8)((nOpcode >>  6) & 0x001F);

	uint8 nBc	= (uint8)((nOpcode >>  0) & 0x0003);
	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, VF%i%s", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sBroadcast[nBc]);
}

void VUShared::ReflOpFsDstItDec(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, (--VI%i)", nFS, m_sDestination[nDest], nIT);
}

void VUShared::ReflOpFsDstItInc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, (VI%i++)", nFS, m_sDestination[nDest], nIT);
}

void VUShared::ReflOpFtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);

	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpFtIs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);
	uint8 nFT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%i%s, VI%i", nFT, m_sDestination[nDest], nIS);
}

void VUShared::ReflOpFtDstIsInc(INSTRUCTION*, CMIPS*, uint32, uint32 opcode, char* text, unsigned int count)
{
	auto dest	= static_cast<uint8>((opcode >> 21) & 0x000F);
	auto ft		= static_cast<uint8>((opcode >> 16) & 0x001F);
	auto is		= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VF%i%s, (VI%i++)", ft, m_sDestination[dest], is);
}

void VUShared::ReflOpClip(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%ixyz, VF%iw", nFS, nFT);
}

void VUShared::ReflOpAccFsI(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, I", m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpAccFsQ(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nDest	= static_cast<uint8>((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, Q", m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpAccFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);

	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpAccFsFtBc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);

	uint8 nBc	= (uint8)((nOpcode >>  0) & 0x0003);
	uint8 nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sBroadcast[nBc]);
}

void VUShared::ReflOpRFsf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS	= (uint8 )((nOpcode >> 11) & 0x001F);
	uint8 nFSF	= (uint8 )((nOpcode >> 21) & 0x0003);

	sprintf(sText, "R, VF%i%s", nFS, m_sBroadcast[nFSF]);
}

void VUShared::ReflOpFtR(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nDest	= (uint8 )((nOpcode >> 21) & 0x000F);
	uint8 nFT	= (uint8 )((nOpcode >> 16) & 0x001F);

	sprintf(sText, "VF%i%s, R", nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpQFtf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);

	uint8 nFTF	= (uint8)((nOpcode >> 23) & 0x0003);

	sprintf(sText, "Q, VF%i%s", nFT, m_sBroadcast[nFTF]);
}

void VUShared::ReflOpQFsfFtf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT	= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS	= (uint8)((nOpcode >> 11) & 0x001F);

	uint8 nFTF	= (uint8)((nOpcode >> 23) & 0x0003);
	uint8 nFSF	= (uint8)((nOpcode >> 21) & 0x0003);

	sprintf(sText, "Q, VF%i%s, VF%i%s", nFS, m_sBroadcast[nFSF], nFT, m_sBroadcast[nFTF]);
}

void VUShared::ReflOpIdIsIt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT = static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nIS = static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nID = static_cast<uint8>((nOpcode >>  6) & 0x001F);

	sprintf(sText, "VI%i, VI%i, VI%i", nID, nIS, nIT);
}

void VUShared::ReflOpItIsDst(INSTRUCTION* instr, CMIPS* context, uint32 address, uint32 opcode, char* text, unsigned int count)
{
	uint8 dest	= static_cast<uint8>((opcode >> 21) & 0x000F);
	uint8 it	= static_cast<uint8>((opcode >> 16) & 0x001F);
	uint8 is	= static_cast<uint8>((opcode >> 11) & 0x001F);

	sprintf(text, "VI%i, (VI%i)%s", it, is, m_sDestination[dest]);
}

void VUShared::ReflOpItIsImm5(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8  nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8  nIS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint16 nImm	= static_cast<uint8>((nOpcode >>  6) & 0x001F);
	if(nImm & 0x10)
	{
		nImm |= 0xFFE0;
	}

	sprintf(sText, "VI%i, VI%i, $%0.4X", nIT, nIS, nImm);
}

void VUShared::ReflOpItFsf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nIT	= static_cast<uint8>((nOpcode >> 16) & 0x001F);
	uint8 nFS	= static_cast<uint8>((nOpcode >> 11) & 0x001F);
	uint8 nFSF	= static_cast<uint8>((nOpcode >> 21) & 0x0003);

	sprintf(sText, "VI%i, VF%i%s", nIT, nFS, m_sBroadcast[nFSF]);
}

////////////////////////////////////////////

VUShared::VUINSTRUCTION* VUShared::DereferenceInstruction(VUSUBTABLE* pSubTable, uint32 nOpcode)
{
	unsigned int nIndex = (nOpcode >> pSubTable->nShift) & pSubTable->nMask;
	return &(pSubTable->pTable[nIndex]);
}

void VUShared::SubTableAffectedOperands(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	pInstr = DereferenceInstruction(pInstr->subTable, nOpcode);
	if(pInstr->pGetAffectedOperands == nullptr)
	{
		//We should always have something that tells us what is affected (even if it's nothing)
		assert(0);
		return;
	}
	pInstr->pGetAffectedOperands(pInstr, pCtx, nAddress, nOpcode, operandSet);
}

void VUShared::ReflOpAffNone(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	//Nothing is affected
}

void VUShared::ReflOpAffAccFsI(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	operandSet.readF0 = nFS;
}

void VUShared::ReflOpAffFdFsQ(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	operandSet.readF0 = nFS;
	operandSet.writeF = nFD;
	operandSet.readQ = true;
}

void VUShared::ReflOpAffFdFsI(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	uint8 nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	operandSet.readF0 = nFS;
	operandSet.writeF = nFD;
}

void VUShared::ReflOpAffFtFs(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	uint8 nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	operandSet.readF0 = nFS;
	operandSet.writeF = nFT;
}

void VUShared::ReflOpAffRFsf(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFS		= (uint8 )((nOpcode >> 11) & 0x001F);
	operandSet.readF0 = nFS;
}

void VUShared::ReflOpAffFtR(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	uint8 nFT		= (uint8 )((nOpcode >> 16) & 0x001F);

	operandSet.writeF = nFT;
}

void VUShared::ReflOpAffQ(VUINSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, OPERANDSET& operandSet)
{
	operandSet.syncQ = true;
}

void VUShared::ReflOpAffWrARdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	//TODO: Write A
	operandSet.readF0 = ft;
	operandSet.readF1 = fs;
}

void VUShared::ReflOpAffWrARdFsQ(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	//TODO: Write A
	operandSet.readF0 = fs;
	operandSet.readQ = true;
}

void VUShared::ReflOpAffWrCfRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);

	//TODO: Write CF
	operandSet.readF0 = ft;
	operandSet.readF1 = fs;
}

void VUShared::ReflOpAffWrFdRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);
	auto fd = static_cast<uint8>((opcode >>  6) & 0x001F);

	operandSet.writeF = fd;
	operandSet.readF0 = ft;
	operandSet.readF1 = fs;
}

void VUShared::ReflOpAffWrQRdFt(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	
	//Would probably be write Q
	operandSet.readF0 = ft;
	operandSet.syncQ = true;
}

void VUShared::ReflOpAffWrQRdFtFs(VUINSTRUCTION*, CMIPS*, uint32, uint32 opcode, OPERANDSET& operandSet)
{
	auto ft = static_cast<uint8>((opcode >> 16) & 0x001F);
	auto fs = static_cast<uint8>((opcode >> 11) & 0x001F);
	
	//Would probably be write Q
	operandSet.readF0 = ft;
	operandSet.readF1 = fs;
	operandSet.syncQ = true;
}
