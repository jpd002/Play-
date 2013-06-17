#include <stdio.h>
#include <algorithm>
#include "GIF.h"
#include "uint128.h"
#include "Ps2Const.h"
#include "Profiler.h"
#include "Log.h"

#define LOG_NAME			("gif")

#ifdef	PROFILE
#define PROFILE_GIFZONE		"GIF"
#endif

CGIF::CGIF(CGSHandler*& gs, uint8* ram, uint8* spr)
: m_gs(gs)
, m_ram(ram)
, m_spr(spr)
, m_nLoops(0)
, m_nCmd(0)
, m_nRegs(0)
, m_nRegsTemp(0)
, m_nRegList(0)
, m_nEOP(false)
, m_nQTemp(0)
{

}

CGIF::~CGIF()
{

}

void CGIF::Reset()
{
	m_nLoops = 0;
	m_nCmd = 0;
	m_nEOP = false;
}

uint32 CGIF::ProcessPacked(CGSHandler::RegisterWriteList& writeList, uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint32 nStart = nAddress;

	while((m_nLoops != 0) && (nAddress < nEnd))
	{
		while((m_nRegsTemp != 0) && (nAddress < nEnd))
		{
			uint64 nTemp;
			uint32 nRegDesc = (uint32)((m_nRegList >> ((m_nRegs - m_nRegsTemp) * 4)) & 0x0F);

			uint128 nPacket = *(uint128*)&pMemory[nAddress];
			nAddress += 0x10;

			m_nRegsTemp--;

			switch(nRegDesc)
			{
			case 0x00:
				//PRIM
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, nPacket.nV0));
				break;
			case 0x01:
				//RGBA
				nTemp  = (nPacket.nV[0] & 0xFF);
				nTemp |= (nPacket.nV[1] & 0xFF) << 8;
				nTemp |= (nPacket.nV[2] & 0xFF) << 16;
				nTemp |= (nPacket.nV[3] & 0xFF) << 24;
				nTemp |= ((uint64)m_nQTemp << 32);
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_RGBAQ, nTemp));
				break;
			case 0x02:
				//ST
				m_nQTemp = nPacket.nV2;
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_ST, nPacket.nD0));
				break;
			case 0x03:
				//UV
				nTemp  = (nPacket.nV[0] & 0x7FFF);
				nTemp |= (nPacket.nV[1] & 0x7FFF) << 16;
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_UV, nTemp));
				break;
			case 0x04:
				//XYZF2
				nTemp  = (nPacket.nV[0] & 0xFFFF);
				nTemp |= (nPacket.nV[1] & 0xFFFF) << 16;
				nTemp |= (uint64)(nPacket.nV[2] & 0x0FFFFFF0) << 28;
				nTemp |= (uint64)(nPacket.nV[3] & 0x00000FF0) << 52;
				if(nPacket.nV[3] & 0x8000)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZF3, nTemp));
				}
				else
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZF2, nTemp));
				}
				break;
			case 0x05:
				//XYZ2
				nTemp  = (nPacket.nV[0] & 0xFFFF);
				nTemp |= (nPacket.nV[1] & 0xFFFF) << 16;
				nTemp |= (uint64)(nPacket.nV[2] & 0xFFFFFFFF) << 32;
				if(nPacket.nV[3] & 0x8000)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, nTemp));
				}
				else
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ2, nTemp));
				}
				break;
			case 0x06:
				//TEX0_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_1, nPacket.nD0));
				break;
			case 0x07:
				//TEX0_2
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_2, nPacket.nD0));
				break;
			case 0x08:
				//CLAMP_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_CLAMP_1, nPacket.nD0));
				break;
			case 0x0D:
				//XYZ3
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, nPacket.nD0));
				break;
			case 0x0E:
				//A + D
				if(m_gs != NULL)
				{
					writeList.push_back(CGSHandler::RegisterWrite(static_cast<uint8>(nPacket.nD1), nPacket.nD0));
				}
				break;
			case 0x0F:
				//NOP
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

uint32 CGIF::ProcessRegList(CGSHandler::RegisterWriteList& writeList, uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint32 nStart = nAddress;

	while(m_nLoops != 0)
	{
		for(uint32 j = 0; j < m_nRegs; j++)
		{
			assert(nAddress < nEnd);

			uint128 nPacket;
			uint32 nRegDesc = (uint32)((m_nRegList >> (j * 4)) & 0x0F);

 			nPacket.nV[0] = *(uint32*)&pMemory[nAddress + 0x00];
			nPacket.nV[1] = *(uint32*)&pMemory[nAddress + 0x04];
			nAddress += 0x08;

			if(nRegDesc == 0x0F) continue;

			writeList.push_back(CGSHandler::RegisterWrite(static_cast<uint8>(nRegDesc), nPacket.nD0));
		}

		m_nLoops--;
	}

	//Align on qword boundary
	if(nAddress & 0x0F)
	{
		nAddress += 8;
	}

	return nAddress - nStart;
}

uint32 CGIF::ProcessImage(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	uint16 nTotalLoops = static_cast<uint16>((nEnd - nAddress) / 0x10);
	nTotalLoops = std::min<uint16>(nTotalLoops, m_nLoops);

	if(m_gs != NULL)
	{
		m_gs->FeedImageData(pMemory + nAddress, nTotalLoops * 0x10);
	}

	m_nLoops -= nTotalLoops;

	return (nTotalLoops * 0x10);
}

uint32 CGIF::ProcessPacket(uint8* pMemory, uint32 nAddress, uint32 nEnd)
{
	std::unique_lock<std::mutex> pathLock(m_pathMutex);
	static CGSHandler::RegisterWriteList writeList;

#ifdef PROFILE
	CProfilerZone profilerZone(PROFILE_GIFZONE);
#endif

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Received GIF packet at 0x%0.8X of 0x%0.8X bytes.\r\n", nAddress, nEnd - nAddress);
#endif

	writeList.clear();

	uint32 nStart = nAddress;
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
			uint128 nPacket = *reinterpret_cast<uint128*>(&pMemory[nAddress]);
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
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, (uint16)(nPacket.nV1 >> 15) & 0x3FF));
				}
			}

			if(m_nRegs == 0) m_nRegs = 0x10;
			m_nRegsTemp = m_nRegs;
			continue;
		}
		switch(m_nCmd)
		{
		case 0x00:
			nAddress += ProcessPacked(writeList, pMemory, nAddress, nEnd);
			break;
		case 0x01:
			nAddress += ProcessRegList(writeList, pMemory, nAddress, nEnd);
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

	if(m_gs != NULL && !writeList.empty())
	{
		m_gs->WriteRegisterMassively(writeList.data(), static_cast<unsigned int>(writeList.size()));
	}

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Processed 0x%0.8X bytes.\r\n", nAddress - nStart);
#endif

	return nAddress - nStart;
}

uint32 CGIF::ReceiveDMA(uint32 nAddress, uint32 nQWC, uint32 unused, bool nTagIncluded)
{
	uint8* pMemory(NULL);
	uint32 nSize = nQWC * 0x10;

	if(nTagIncluded)
	{
		assert(nQWC >= 0);
		nSize -= 0x10;
		nAddress += 0x10;
	}

	if(nAddress & 0x80000000)
	{
		pMemory = m_spr;
		nAddress &= PS2::EE_SPR_SIZE - 1;
	}
	else
	{
		pMemory = m_ram;
	}
	
	uint32 nEnd = nAddress + nSize;

	while(nAddress < nEnd)
	{
		nAddress += ProcessPacket(pMemory, nAddress, nEnd);
	}

	return nQWC;
}
