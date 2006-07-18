#include <stdio.h>
#include "GIF.h"
#include "uint128.h"
#include "PS2VM.h"
#include "Profiler.h"

using namespace Framework;

#ifdef	PROFILE
#define PROFILE_GIFZONE		"GIF"
#endif

uint16	CGIF::m_nLoops		= 0;
uint8	CGIF::m_nCmd		= 0;
uint8	CGIF::m_nRegs		= 0;
uint8	CGIF::m_nRegsTemp	= 0;
uint64	CGIF::m_nRegList	= 0;
bool	CGIF::m_nEOP		= false;
uint32	CGIF::m_nQTemp		= 0;

void CGIF::Reset()
{
	m_nLoops = 0;
	m_nCmd = 0;
	m_nEOP = false;
}

uint32 CGIF::ProcessPacked(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint32 nRegDesc, nStart;
	uint64 nTemp;
	uint128 nPacket;

	nStart = nAddress;

	while((m_nLoops != 0) && (nAddress < nEnd))
	{
		while((m_nRegsTemp != 0) && (nAddress < nEnd))
		{
			nRegDesc = (uint32)((m_nRegList >> ((m_nRegs - m_nRegsTemp) * 4)) & 0x0F);

			nPacket = *(uint128*)&pMemory[nAddress];
			nAddress += 0x10;

			m_nRegsTemp--;

			switch(nRegDesc)
			{
			case 0x01:
				//RGBA
				nTemp  = (nPacket.nV[0] & 0xFF);
				nTemp |= (nPacket.nV[1] & 0xFF) << 8;
				nTemp |= (nPacket.nV[2] & 0xFF) << 16;
				nTemp |= (nPacket.nV[3] & 0xFF) << 24;
				nTemp |= ((uint64)m_nQTemp << 32);
				CPS2VM::m_pGS->WriteRegister(GS_REG_RGBAQ, nTemp);
				break;
			case 0x02:
				//ST
				m_nQTemp = nPacket.nV2;
				CPS2VM::m_pGS->WriteRegister(GS_REG_ST, nPacket.nD0);
				break;
			case 0x03:
				//UV
				nTemp  = (nPacket.nV[0] & 0x7FFF);
				nTemp |= (nPacket.nV[1] & 0x7FFF) << 16;
				CPS2VM::m_pGS->WriteRegister(GS_REG_UV, nTemp);
				break;
			case 0x04:
				//XYZF2
				nTemp  = (nPacket.nV[0] & 0xFFFF);
				nTemp |= (nPacket.nV[1] & 0xFFFF) << 16;
				nTemp |= (uint64)(nPacket.nV[2] & 0x0FFFFFF0) << 28;
				nTemp |= (uint64)(nPacket.nV[3] & 0x00000FF0) << 52;
				if(nPacket.nV[3] & 0x8000)
				{
					CPS2VM::m_pGS->WriteRegister(GS_REG_XYZF3, nTemp);
				}
				else
				{
					CPS2VM::m_pGS->WriteRegister(GS_REG_XYZF2, nTemp);
				}
				break;
			case 0x05:
				//XYZ2
				nTemp  = (nPacket.nV[0] & 0xFFFF);
				nTemp |= (nPacket.nV[1] & 0xFFFF) << 16;
				nTemp |= (uint64)(nPacket.nV[2] & 0xFFFFFFFF) << 32;
				if(nPacket.nV[3] & 0x8000)
				{
					CPS2VM::m_pGS->WriteRegister(GS_REG_XYZ3, nTemp);
				}
				else
				{
					CPS2VM::m_pGS->WriteRegister(GS_REG_XYZ2, nTemp);
				}
				break;
			case 0x06:
				//TEX0_1
				CPS2VM::m_pGS->WriteRegister(GS_REG_TEX0_1, nPacket.nD0);
				break;
			case 0x0E:
				//A + D
				CPS2VM::m_pGS->WriteRegister((uint8)nPacket.nD1, nPacket.nD0);
				break;
			default:
				assert(0);
				break;
			}
		}

		if(m_nRegsTemp == 0)
		{
			m_nLoops--;
			m_nRegsTemp = m_nRegs;
		}

	}

	return nAddress - nStart;
}

uint32 CGIF::ProcessRegList(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint32 nRegDesc, j, nStart;
	uint128 nPacket;

	nStart = nAddress;

	while(m_nLoops != 0)
	{
		for(j = 0; j < m_nRegs; j++)
		{
			assert(nAddress < nEnd);

			nRegDesc = (uint32)((m_nRegList >> (j * 4)) & 0x0F);

 			nPacket.nV[0] = *(uint32*)&pMemory[nAddress + 0x00];
			nPacket.nV[1] = *(uint32*)&pMemory[nAddress + 0x04];
			nAddress += 0x08;

			if(nRegDesc == 0x0F) continue;

			CPS2VM::m_pGS->WriteRegister((uint8)nRegDesc, nPacket.nD0);
		}

		m_nLoops--;
	}

	//Align on qword boundary
	if(nAddress & 0x07)
	{
		nAddress += 8;
	}

	return nAddress - nStart;
}

uint32 CGIF::ProcessImage(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint16 nTotalLoops;
	
	nTotalLoops = (uint16)((nEnd - nAddress) / 0x10);
	nTotalLoops = min(nTotalLoops, m_nLoops);

	CPS2VM::m_pGS->FeedImageData(pMemory + nAddress, nTotalLoops * 0x10);

	m_nLoops -= nTotalLoops;

	return (nTotalLoops * 0x10);
}

uint32 CGIF::ProcessPacket(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_GIFZONE);
#endif

	uint128 nPacket;
	uint32 nStart;

	if(CPS2VM::m_Logging.GetGSLoggingStatus())
	{
		printf("GIF: Processed GIF packet at 0x%0.8X, PC: 0x%0.8X.\r\n", nAddress, CPS2VM::m_EE.m_State.nPC);
	}

	nStart = nAddress;
	while(nAddress < nEnd)
	{
		if(m_nLoops == 0)
		{
			if(m_nEOP)
			{
				m_nEOP = false;
				break;
			}

			//We need to update the registers
			nPacket = *(uint128*)&pMemory[nAddress];
			nAddress += 0x10;

			m_nLoops	= (uint16)((nPacket.nV0 >>  0) & 0x7FFF);
			m_nCmd		= ( uint8)((nPacket.nV1 >> 26) & 0x0003);
			m_nRegs		= ( uint8)((nPacket.nV1 >> 28) & 0x000F);
			m_nRegList	= nPacket.nD1;
			m_nEOP		= (nPacket.nV0 & 0x8000) != 0;

			if(m_nCmd != 1)
			{
				if(nPacket.nV1 & 0x4000)
				{
					CPS2VM::m_pGS->WriteRegister(GS_REG_PRIM, (uint16)(nPacket.nV1 >> 15) & 0x3FF);
				}
			}

			if(m_nRegs == 0) m_nRegs = 0x10;
			m_nRegsTemp = m_nRegs;
			continue;
		}
		switch(m_nCmd)
		{
		case 0x00:
			nAddress += ProcessPacked(pMemory, nAddress, nEnd);
			break;
		case 0x01:
			nAddress += ProcessRegList(pMemory, nAddress, nEnd);
			break;
		case 0x02:
		case 0x03:
			nAddress += ProcessImage(pMemory, nAddress, nEnd);
			break;
		}
	}

	if(m_nLoops == 0)
	{
		if(m_nEOP)
		{
			m_nEOP = false;
		}
	}

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	return nAddress - nStart;
}

uint32 CGIF::ReceiveDMA(uint32 nAddress, uint32 nQWC, bool nTagIncluded)
{
	uint32 nEnd, nSize;
	uint8* pMemory;

	assert(nTagIncluded == false);

	if(nAddress & 0x80000000)
	{
		pMemory = CPS2VM::m_pSPR;
		nAddress &= CPS2VM::SPRSIZE - 1;
	}
	else
	{
		pMemory = CPS2VM::m_pRAM;
	}
	
	nSize = nQWC * 0x10;
	nEnd = nAddress + nSize;

	while(nAddress < nEnd)
	{
		nAddress += ProcessPacket(pMemory, nAddress, nEnd);
	}

	return nQWC;
}
