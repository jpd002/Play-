#include <stdio.h>
#include <algorithm>
#include "../uint128.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "../FrameDump.h"
#include "GIF.h"

#define LOG_NAME ("gif")

CGIF::CGIF(CGSHandler*& gs, uint8* ram, uint8* spr)
: m_gs(gs)
, m_ram(ram)
, m_spr(spr)
, m_loops(0)
, m_cmd(0)
, m_regs(0)
, m_regsTemp(0)
, m_regList(0)
, m_eop(false)
, m_qtemp(0)
, m_gifProfilerZone(CProfiler::GetInstance().RegisterZone("GIF"))
{

}

CGIF::~CGIF()
{

}

void CGIF::Reset()
{
	m_loops = 0;
	m_cmd = 0;
	m_regs = 0;
	m_regsTemp = 0;
	m_regList = 0;
	m_eop = false;
	m_qtemp = 0;
}

uint32 CGIF::ProcessPacked(CGSHandler::RegisterWriteList& writeList, uint8* memory, uint32 address, uint32 end)
{
	uint32 start = address;

	while((m_loops != 0) && (address < end))
	{
		while((m_regsTemp != 0) && (address < end))
		{
			uint64 temp = 0;
			uint32 regDesc = (uint32)((m_regList >> ((m_regs - m_regsTemp) * 4)) & 0x0F);

			uint128 packet = *(uint128*)&memory[address];
			address += 0x10;

			m_regsTemp--;

			switch(regDesc)
			{
			case 0x00:
				//PRIM
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, packet.nV0));
				break;
			case 0x01:
				//RGBA
				temp  = (packet.nV[0] & 0xFF);
				temp |= (packet.nV[1] & 0xFF) << 8;
				temp |= (packet.nV[2] & 0xFF) << 16;
				temp |= (packet.nV[3] & 0xFF) << 24;
				temp |= ((uint64)m_qtemp << 32);
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_RGBAQ, temp));
				break;
			case 0x02:
				//ST
				m_qtemp = packet.nV2;
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_ST, packet.nD0));
				break;
			case 0x03:
				//UV
				temp  = (packet.nV[0] & 0x7FFF);
				temp |= (packet.nV[1] & 0x7FFF) << 16;
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_UV, temp));
				break;
			case 0x04:
				//XYZF2
				temp  = (packet.nV[0] & 0xFFFF);
				temp |= (packet.nV[1] & 0xFFFF) << 16;
				temp |= (uint64)(packet.nV[2] & 0x0FFFFFF0) << 28;
				temp |= (uint64)(packet.nV[3] & 0x00000FF0) << 52;
				if(packet.nV[3] & 0x8000)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZF3, temp));
				}
				else
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZF2, temp));
				}
				break;
			case 0x05:
				//XYZ2
				temp  = (packet.nV[0] & 0xFFFF);
				temp |= (packet.nV[1] & 0xFFFF) << 16;
				temp |= (uint64)(packet.nV[2] & 0xFFFFFFFF) << 32;
				if(packet.nV[3] & 0x8000)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, temp));
				}
				else
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ2, temp));
				}
				break;
			case 0x06:
				//TEX0_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_1, packet.nD0));
				break;
			case 0x07:
				//TEX0_2
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_TEX0_2, packet.nD0));
				break;
			case 0x08:
				//CLAMP_1
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_CLAMP_1, packet.nD0));
				break;
			case 0x0D:
				//XYZ3
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_XYZ3, packet.nD0));
				break;
			case 0x0E:
				//A + D
				writeList.push_back(CGSHandler::RegisterWrite(static_cast<uint8>(packet.nD1), packet.nD0));
				break;
			case 0x0F:
				//NOP
				break;
			default:
				assert(0);
				break;
			}
		}

		if(m_regsTemp == 0)
		{
			m_loops--;
			m_regsTemp = m_regs;
		}

	}

	return address - start;
}

uint32 CGIF::ProcessRegList(CGSHandler::RegisterWriteList& writeList, uint8* memory, uint32 address, uint32 end)
{
	uint32 start = address;

	while(m_loops != 0)
	{
		for(uint32 j = 0; j < m_regs; j++)
		{
			assert(address < end);

			uint128 packet;
			uint32 nRegDesc = (uint32)((m_regList >> (j * 4)) & 0x0F);

			packet.nV[0] = *(uint32*)&memory[address + 0x00];
			packet.nV[1] = *(uint32*)&memory[address + 0x04];
			address += 0x08;

			if(nRegDesc == 0x0F) continue;

			writeList.push_back(CGSHandler::RegisterWrite(static_cast<uint8>(nRegDesc), packet.nD0));
		}

		m_loops--;
	}

	//Align on qword boundary
	if(address & 0x0F)
	{
		address += 8;
	}

	return address - start;
}

uint32 CGIF::ProcessImage(uint8* memory, uint32 address, uint32 end)
{
	uint16 totalLoops = static_cast<uint16>((end - address) / 0x10);
	totalLoops = std::min<uint16>(totalLoops, m_loops);

	if(m_gs != NULL)
	{
		m_gs->FeedImageData(memory + address, totalLoops * 0x10);
	}

	m_loops -= totalLoops;

	return (totalLoops * 0x10);
}

uint32 CGIF::ProcessPacket(uint8* memory, uint32 address, uint32 end, const CGsPacketMetadata& packetMetadata)
{
	std::unique_lock<std::mutex> pathLock(m_pathMutex);
	static CGSHandler::RegisterWriteList writeList;
	static const auto flushWriteList =
		[] (CGSHandler* gs, const CGsPacketMetadata& packetMetadata)
		{
			if((gs != nullptr) && !writeList.empty())
			{
				gs->WriteRegisterMassively(writeList.data(), static_cast<unsigned int>(writeList.size()), &packetMetadata);
			}
			writeList.clear();
		};

#ifdef PROFILE
	CProfilerZone profilerZone(m_gifProfilerZone);
#endif

#if defined(_DEBUG) && defined(DEBUGGER_INCLUDED)
	CLog::GetInstance().Print(LOG_NAME, "Received GIF packet on path %d at 0x%0.8X of 0x%0.8X bytes.\r\n", 
		packetMetadata.pathIndex, address, end - address);
#endif

	writeList.clear();

	uint32 start = address;
	while(address < end)
	{
		if(m_loops == 0)
		{
			if(m_eop)
			{
				m_eop = false;
				break;
			}

			//We need to update the registers
			TAG tag = *reinterpret_cast<TAG*>(&memory[address]);
			address += 0x10;

			m_loops		= tag.loops;
			m_cmd		= tag.cmd;
			m_regs		= tag.nreg;
			m_regList	= tag.regs;
			m_eop		= (tag.eop != 0);

			if(m_cmd != 1)
			{
				if(tag.pre != 0)
				{
					writeList.push_back(CGSHandler::RegisterWrite(GS_REG_PRIM, static_cast<uint64>(tag.prim)));
				}
			}

			if(m_regs == 0) m_regs = 0x10;
			m_regsTemp = m_regs;
			continue;
		}
		switch(m_cmd)
		{
		case 0x00:
			address += ProcessPacked(writeList, memory, address, end);
			break;
		case 0x01:
			address += ProcessRegList(writeList, memory, address, end);
			break;
		case 0x02:
		case 0x03:
			//We need to flush our list here because image data can be embedded in a GIF packet
			//that specifies pixel transfer information in GS registers (and that has to be send first)
			//This is done by FFX
			flushWriteList(m_gs, packetMetadata);
			address += ProcessImage(memory, address, end);
			break;
		}
	}

	if(m_loops == 0)
	{
		if(m_eop)
		{
			m_eop = false;
		}
	}

	flushWriteList(m_gs, packetMetadata);

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Processed 0x%0.8X bytes.\r\n", address - start);
#endif

	return address - start;
}

uint32 CGIF::ReceiveDMA(uint32 address, uint32 qwc, uint32 unused, bool tagIncluded)
{
	uint8* memory(nullptr);
	uint32 size = qwc * 0x10;

	if(tagIncluded)
	{
		assert(qwc >= 0);
		size -= 0x10;
		address += 0x10;
	}

	if(address & 0x80000000)
	{
		memory = m_spr;
		address &= PS2::EE_SPR_SIZE - 1;
	}
	else
	{
		memory = m_ram;
	}
	
	uint32 end = address + size;

	while(address < end)
	{
		address += ProcessPacket(memory, address, end, CGsPacketMetadata(3));
	}

	return qwc;
}

uint32 CGIF::GetRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case GIF_STAT:
		if(m_gs->GetPendingTransferCount() != 0)
		{
			result |= GIF_STAT_APATH3;
		}
		break;
	}
#ifdef _DEBUG
	DisassembleGet(address);
#endif
	return result;
}

void CGIF::SetRegister(uint32 address, uint32 value)
{
#ifdef _DEBUG
	DisassembleSet(address, value);
#endif
}

void CGIF::DisassembleGet(uint32 address)
{
	switch(address)
	{
	case GIF_STAT:
		CLog::GetInstance().Print(LOG_NAME, "= GIF_STAT.\r\n", address);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading unknown register 0x%0.8X.\r\n", address);
		break;
	}
}

void CGIF::DisassembleSet(uint32 address, uint32 value)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Writing unknown register 0x%0.8X, 0x%0.8X.\r\n", address, value);
		break;
	}
}
