#include <stdio.h>
#include <algorithm>
#include "../uint128.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "../FrameDump.h"
#include "../RegisterStateFile.h"
#include "GIF.h"

#define LOG_NAME ("gif")

#define STATE_REGS_XML        ("gif/regs.xml")
#define STATE_REGS_M3P        ("M3P")
#define STATE_REGS_ACTIVEPATH ("ActivePath")
#define STATE_REGS_LOOPS      ("LOOPS")
#define STATE_REGS_CMD        ("CMD")
#define STATE_REGS_REGS       ("REGS")
#define STATE_REGS_REGSTEMP   ("REGSTEMP")
#define STATE_REGS_REGLIST    ("REGLIST")
#define STATE_REGS_EOP        ("EOP")
#define STATE_REGS_QTEMP      ("QTEMP")

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
	m_path3Masked = false;
	m_activePath = 0;
	m_loops = 0;
	m_cmd = 0;
	m_regs = 0;
	m_regsTemp = 0;
	m_regList = 0;
	m_eop = false;
	m_qtemp = 0;
}

void CGIF::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_path3Masked = registerFile.GetRegister32(STATE_REGS_M3P) != 0;
	m_activePath  = registerFile.GetRegister32(STATE_REGS_ACTIVEPATH);
	m_loops       = static_cast<uint16>(registerFile.GetRegister32(STATE_REGS_LOOPS));
	m_cmd         = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_CMD));
	m_regs        = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_REGS));
	m_regsTemp    = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_REGSTEMP));
	m_regList     = registerFile.GetRegister64(STATE_REGS_REGLIST);
	m_eop         = registerFile.GetRegister32(STATE_REGS_EOP) != 0;
	m_qtemp       = registerFile.GetRegister32(STATE_REGS_QTEMP);
}

void CGIF::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister32(STATE_REGS_M3P,        m_path3Masked ? 1 : 0);
	registerFile->SetRegister32(STATE_REGS_ACTIVEPATH, m_activePath);
	registerFile->SetRegister32(STATE_REGS_LOOPS,      m_loops);
	registerFile->SetRegister32(STATE_REGS_CMD,        m_cmd);
	registerFile->SetRegister32(STATE_REGS_REGS,       m_regs);
	registerFile->SetRegister32(STATE_REGS_REGSTEMP,   m_regsTemp);
	registerFile->SetRegister64(STATE_REGS_REGLIST,    m_regList);
	registerFile->SetRegister32(STATE_REGS_EOP,        m_eop ? 1 : 0);
	registerFile->SetRegister32(STATE_REGS_QTEMP,      m_qtemp);
	archive.InsertFile(registerFile);
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
			case 0x0A:
				//FOG
				writeList.push_back(CGSHandler::RegisterWrite(GS_REG_FOG, (packet.nD1 >> 36) << 56));
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
		if(address == end)
		{
			break;
		}

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
	m_gs->FeedImageData(memory + address, totalLoops * 0x10);
	m_loops -= totalLoops;

	return (totalLoops * 0x10);
}

uint32 CGIF::ProcessSinglePacket(uint8* memory, uint32 address, uint32 end, const CGsPacketMetadata& packetMetadata)
{
	static CGSHandler::RegisterWriteList writeList;
	static const auto flushWriteList =
		[] (CGSHandler* gs, const CGsPacketMetadata& packetMetadata)
		{
			if(!writeList.empty())
			{
				auto currentCapacity = writeList.capacity();
				gs->WriteRegisterMassively(std::move(writeList), &packetMetadata);
				writeList.reserve(currentCapacity);
			}
		};

#ifdef PROFILE
	CProfilerZone profilerZone(m_gifProfilerZone);
#endif

#if defined(_DEBUG) && defined(DEBUGGER_INCLUDED)
	CLog::GetInstance().Print(LOG_NAME, "Received GIF packet on path %d at 0x%0.8X of 0x%0.8X bytes.\r\n", 
		packetMetadata.pathIndex, address, end - address);
#endif

	assert((m_activePath == 0) || (m_activePath == packetMetadata.pathIndex));
	writeList.clear();

	uint32 start = address;
	while(address < end)
	{
		if(m_loops == 0)
		{
			if(m_eop)
			{
				m_eop = false;
				m_activePath = 0;
				break;
			}

			//We need to update the registers
			auto tag = *reinterpret_cast<TAG*>(&memory[address]);
			address += 0x10;
#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "TAG(loops = %d, eop = %d, pre = %d, prim = 0x%0.4X, cmd = %d, nreg = %d);\r\n",
				tag.loops, tag.eop, tag.pre, tag.prim, tag.cmd, tag.nreg);
#endif

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
			m_activePath = packetMetadata.pathIndex;
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
			m_activePath = 0;
		}
	}

	flushWriteList(m_gs, packetMetadata);

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Processed 0x%0.8X bytes.\r\n", address - start);
#endif

	return address - start;
}

uint32 CGIF::ProcessMultiplePackets(uint8* memory, uint32 address, uint32 end, const CGsPacketMetadata& packetMetadata)
{
	//This will attempt to process everything from [address, end[ even if it contains multiple GIF packets

	if((m_activePath != 0) && (m_activePath != packetMetadata.pathIndex))
	{
		//Packet transfer already active on a different path, we can't process this one
		return 0;
	}

	uint32 start = address;
	while(address < end)
	{
		address += ProcessSinglePacket(memory, address, end, packetMetadata);
	}
	assert(address <= end);
	return address - start;
}

uint32 CGIF::ReceiveDMA(uint32 address, uint32 qwc, uint32 unused, bool tagIncluded)
{
	uint32 size = qwc * 0x10;

	uint8* memory = nullptr;
	if(address & 0x80000000)
	{
		memory = m_spr;
		address &= PS2::EE_SPR_SIZE - 1;
		assert((address + size) <= PS2::EE_SPR_SIZE);
	}
	else
	{
		memory = m_ram;
	}

	uint32 start = address;
	uint32 end = address + size;

	if(tagIncluded)
	{
		assert(qwc >= 0);
		address += 0x10;
	}
	
	address += ProcessMultiplePackets(memory, address, end, CGsPacketMetadata(3));
	assert(address <= end);

	return (address - start) / 0x10;
}

uint32 CGIF::GetRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case GIF_STAT:
		if(m_path3Masked)
		{
			result |= GIF_STAT_M3P;
			//Indicate that FIFO is full (15 qwords) (needed for GTA: San Andreas)
			result |= (0x1F << 24);
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

CGSHandler* CGIF::GetGsHandler()
{
	return m_gs;
}

void CGIF::SetPath3Masked(bool masked)
{
	m_path3Masked = masked;
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
