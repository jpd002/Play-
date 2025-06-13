#include <cassert>
#include <algorithm>
#include "string_format.h"
#include "../states/RegisterStateFile.h"
#include "../FrameDump.h"
#include "GIF.h"
#include "Dmac_Channel.h"
#include "Vpu.h"
#include "Vif1.h"

#define STATE_PATH_FORMAT ("vpu/vif1_%d.xml")
#define STATE_REGS_BASE ("BASE")
#define STATE_REGS_TOP ("TOP")
#define STATE_REGS_TOPS ("TOPS")
#define STATE_REGS_OFST ("OFST")
#define STATE_REGS_DIRECTQWORDBUFFER ("directQwordBuffer")
#define STATE_REGS_DIRECTQWORDBUFFER_INDEX ("directQwordBufferIndex")

CVif1::CVif1(unsigned int number, CVpu& vpu, CGIF& gif, CINTC& intc, uint8* ram, uint8* spr)
    : CVif(1, vpu, intc, ram, spr)
    , m_gif(gif)
{
}

void CVif1::Reset()
{
	CVif::Reset();
	m_BASE = 0;
	m_TOP = 0;
	m_TOPS = 0;
	m_OFST = 0;
	m_directQwordBufferIndex = 0;
	memset(&m_directQwordBuffer, 0, sizeof(m_directQwordBuffer));
}

void CVif1::SaveState(Framework::CZipArchiveWriter& archive)
{
	CVif::SaveState(archive);

	auto path = string_format(STATE_PATH_FORMAT, m_number);
	auto registerFile = std::make_unique<CRegisterStateFile>(path.c_str());
	registerFile->SetRegister32(STATE_REGS_BASE, m_BASE);
	registerFile->SetRegister32(STATE_REGS_TOP, m_TOP);
	registerFile->SetRegister32(STATE_REGS_TOPS, m_TOPS);
	registerFile->SetRegister32(STATE_REGS_OFST, m_OFST);
	registerFile->SetRegister128(STATE_REGS_DIRECTQWORDBUFFER, *reinterpret_cast<uint128*>(&m_directQwordBuffer));
	registerFile->SetRegister32(STATE_REGS_DIRECTQWORDBUFFER_INDEX, m_directQwordBufferIndex);
	archive.InsertFile(std::move(registerFile));
}

void CVif1::LoadState(Framework::CZipArchiveReader& archive)
{
	CVif::LoadState(archive);

	auto path = string_format(STATE_PATH_FORMAT, m_number);
	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_BASE = registerFile.GetRegister32(STATE_REGS_BASE);
	m_TOP = registerFile.GetRegister32(STATE_REGS_TOP);
	m_TOPS = registerFile.GetRegister32(STATE_REGS_TOPS);
	m_OFST = registerFile.GetRegister32(STATE_REGS_OFST);
	*reinterpret_cast<uint128*>(&m_directQwordBuffer) = registerFile.GetRegister128(STATE_REGS_DIRECTQWORDBUFFER);
	m_directQwordBufferIndex = registerFile.GetRegister32(STATE_REGS_DIRECTQWORDBUFFER_INDEX);
}

uint32 CVif1::GetTOP() const
{
	return m_TOP;
}

uint32 CVif1::ReceiveDMA(uint32 address, uint32 qwc, uint32 direction, bool tagIncluded)
{
	if(direction == Dmac::CChannel::CHCR_DIR_TO)
	{
		uint8* source = nullptr;
		uint32 size = qwc * 0x10;
		if(address & 0x80000000)
		{
			source = m_spr;
			address &= (PS2::EE_SPR_SIZE - 1);
			assert((address + size) <= PS2::EE_SPR_SIZE);
		}
		else
		{
			source = m_ram;
			address &= (PS2::EE_RAM_SIZE - 1);
			assert((address + size) <= PS2::EE_RAM_SIZE);
		}
		auto gs = m_gif.GetGsHandler();
		gs->ReadImageData(source + address, size);
		return qwc;
	}
	else
	{
		return CVif::ReceiveDMA(address, qwc, direction, tagIncluded);
	}
}

void CVif1::ExecuteCommand(StreamType& stream, CODE nCommand)
{
#ifdef _DEBUG
	DisassembleCommand(nCommand);
#endif
	switch(nCommand.nCMD)
	{
	case CODE_CMD_OFFSET:
		m_OFST = nCommand.nIMM;
		m_STAT.nDBF = 0;
		m_TOPS = m_BASE;
		break;
	case CODE_CMD_BASE:
		m_BASE = nCommand.nIMM;
		break;
	case CODE_CMD_MSKPATH3:
		m_gif.SetPath3Masked((nCommand.nIMM & 0x8000) != 0);
		break;
	case CODE_CMD_FLUSH:
		if(!m_vpu.IsVuReady())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		break;
	case CODE_CMD_FLUSHA:
		if(!m_vpu.IsVuReady())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		break;
	case CODE_CMD_DIRECT:
	case CODE_CMD_DIRECTHL:
		Cmd_DIRECT(stream, nCommand);
		break;
	default:
		CVif::ExecuteCommand(stream, nCommand);
		break;
	}
}

void CVif1::Cmd_DIRECT(StreamType& stream, CODE nCommand)
{
	if(m_STAT.nVPS != 1)
	{
		if(nCommand.nIMM >= 0x10)
		{
			//Delay DMA completion on bigger DIRECT commands
			m_delayDmaCompletion = true;
		}
	}

	uint32 nSize = stream.GetAvailableReadBytes();
	assert((nSize & 0x03) == 0);

	if(nSize != 0)
	{
		//Check if we have data but less than a qword
		//If we do, we have to go inside a different path to complete a full qword
		bool hasPartialQword = (m_directQwordBufferIndex != 0) || (nSize < QWORD_SIZE);
		if(hasPartialQword)
		{
			//Read enough bytes to try to complete our qword
			assert(m_directQwordBufferIndex <= QWORD_SIZE);
			uint32 readAmount = std::min(nSize, QWORD_SIZE - m_directQwordBufferIndex);
			stream.Read(m_directQwordBuffer + m_directQwordBufferIndex, readAmount);
			m_directQwordBufferIndex += readAmount;
			nSize -= readAmount;

			//If our qword is complete, send to GIF
			if(m_directQwordBufferIndex == QWORD_SIZE)
			{
				assert(m_CODE.nIMM != 0);
				uint32 processed = m_gif.ProcessMultiplePackets(m_directQwordBuffer,
				                                                QWORD_SIZE, 0, QWORD_SIZE, CGsPacketMetadata(2));
				if(processed != 0)
				{
					assert(processed == QWORD_SIZE);
					m_CODE.nIMM--;
					m_directQwordBufferIndex = 0;
				}
			}
		}

		//If no data pending in our partial qword buffer, go forward with multiple qword transfer
		if(m_directQwordBufferIndex == 0)
		{
			nSize = std::min<uint32>(m_CODE.nIMM * 0x10, nSize & ~0xF);

			auto packet = stream.GetDirectPointer();
			uint32 processed = m_gif.ProcessMultiplePackets(packet, nSize, 0, nSize, CGsPacketMetadata(2));
			assert(processed <= nSize);
			stream.Advance(processed);
			//Adjust size in case not everything was processed by GIF
			nSize = processed;

			m_CODE.nIMM -= (nSize / 0x10);
		}
	}

	if(m_CODE.nIMM == 0)
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}
}

void CVif1::Cmd_UNPACK(StreamType& stream, CODE nCommand, uint32 nDstAddr)
{
	bool nFlg = (m_CODE.nIMM & 0x8000) != 0;
	if(nFlg)
	{
		nDstAddr += m_TOPS;
	}

	return CVif::Cmd_UNPACK(stream, nCommand, nDstAddr);
}

void CVif1::PrepareMicroProgram()
{
	CVif::PrepareMicroProgram();

	m_TOP = m_TOPS;

	if(m_STAT.nDBF == 1)
	{
		m_TOPS = m_BASE;
	}
	else
	{
		m_TOPS = m_BASE + m_OFST;
	}
	m_STAT.nDBF = ~m_STAT.nDBF;
}
