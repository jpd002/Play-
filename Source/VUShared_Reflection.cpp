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

void VUShared::ReflOpFdFsI(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFD, nFS;
	uint8 nDest;

	nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, I", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsQ(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFD, nFS;
	uint8 nDest;

	nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, Q", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFD, nFT, nFS;
	uint8 nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, VF%i%s", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpFdFsFtBc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFD, nFT, nFS;
	uint8 nBc, nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);
	nFD		= (uint8)((nOpcode >>  6) & 0x001F);

	nBc		= (uint8)((nOpcode >>  0) & 0x0003);
	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s, VF%i%s", nFD, m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sBroadcast[nBc]);
}

void VUShared::ReflOpFtFs(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "VF%i%s, VF%i%s", nFT, m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpClip(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	sprintf(sText, "VF%ixyz, VF%iw", nFS, nFT);
}

void VUShared::ReflOpAccFsI(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS;
	uint8 nDest;

	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, I", m_sDestination[nDest], nFS, m_sDestination[nDest]);
}

void VUShared::ReflOpAccFsFt(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpAccFsFtBc(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nBc, nDest;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nBc		= (uint8)((nOpcode >>  0) & 0x0003);
	nDest	= (uint8)((nOpcode >> 21) & 0x000F);

	sprintf(sText, "ACC%s, VF%i%s, VF%i%s", m_sDestination[nDest], nFS, m_sDestination[nDest], nFT, m_sBroadcast[nBc]);
}

void VUShared::ReflOpRFsf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFS, nFSF;

	nFS		= (uint8 )((nOpcode >> 11) & 0x001F);
	nFSF	= (uint8 )((nOpcode >> 21) & 0x0003);

	sprintf(sText, "R, VF%i%s", nFS, m_sBroadcast[nFSF]);
}

void VUShared::ReflOpFtR(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nDest;

	nDest	= (uint8 )((nOpcode >> 21) & 0x000F);
	nFT		= (uint8 )((nOpcode >> 16) & 0x001F);

	sprintf(sText, "VF%i%s, R", nFT, m_sDestination[nDest]);
}

void VUShared::ReflOpQFtf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT;
	uint8 nFTF;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);

	nFTF	= (uint8)((nOpcode >> 23) & 0x0003);

	sprintf(sText, "Q, VF%i%s", nFT, m_sBroadcast[nFTF]);
}

void VUShared::ReflOpQFsfFtf(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	uint8 nFT, nFS;
	uint8 nFTF, nFSF;

	nFT		= (uint8)((nOpcode >> 16) & 0x001F);
	nFS		= (uint8)((nOpcode >> 11) & 0x001F);

	nFTF	= (uint8)((nOpcode >> 23) & 0x0003);
	nFSF	= (uint8)((nOpcode >> 21) & 0x0003);

	sprintf(sText, "Q, VF%i%s, VF%i%s", nFS, m_sBroadcast[nFSF], nFT, m_sBroadcast[nFTF]);
}
