#include <cassert>
#include <algorithm>
#include "string_format.h"
#include "../RegisterStateFile.h"
#include "../FrameDump.h"
#include "GIF.h"
#include "Vpu.h"
#include "Vif1.h"

#define STATE_PATH_FORMAT		("vpu/vif1_%d.xml")
#define STATE_REGS_BASE			("BASE")
#define STATE_REGS_TOP			("TOP")
#define STATE_REGS_TOPS			("TOPS")
#define STATE_REGS_OFST			("OFST")

CVif1::CVif1(unsigned int number, CVpu& vpu, CGIF& gif, uint8* ram, uint8* spr)
: CVif(1, vpu, ram, spr)
, m_gif(gif)
{

}

CVif1::~CVif1()
{

}

void CVif1::Reset()
{
	CVif::Reset();
	m_BASE	= 0;
	m_TOP	= 0;
	m_TOPS	= 0;
	m_OFST	= 0;
}

void CVif1::SaveState(Framework::CZipArchiveWriter& archive)
{
	CVif::SaveState(archive);

	auto path = string_format(STATE_PATH_FORMAT, m_number);
	CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
	registerFile->SetRegister32(STATE_REGS_BASE,	m_BASE);
	registerFile->SetRegister32(STATE_REGS_TOP,		m_TOP);
	registerFile->SetRegister32(STATE_REGS_TOPS,	m_TOPS);
	registerFile->SetRegister32(STATE_REGS_OFST,	m_OFST);
	archive.InsertFile(registerFile);
}

void CVif1::LoadState(Framework::CZipArchiveReader& archive)
{
	CVif::LoadState(archive);

	auto path = string_format(STATE_PATH_FORMAT, m_number);
	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_BASE	= registerFile.GetRegister32(STATE_REGS_BASE);
	m_TOP	= registerFile.GetRegister32(STATE_REGS_TOP);
	m_TOPS	= registerFile.GetRegister32(STATE_REGS_TOPS);
	m_OFST	= registerFile.GetRegister32(STATE_REGS_OFST);
}

uint32 CVif1::GetTOP() const
{
	return m_TOP;
}

void CVif1::ExecuteCommand(StreamType& stream, CODE nCommand)
{
#ifdef _DEBUG
	DisassembleCommand(nCommand);
#endif
	switch(nCommand.nCMD)
	{
	case 0x02:
		//OFFSET
		m_OFST = nCommand.nIMM;
		m_STAT.nDBF = 0;
		m_TOPS = m_BASE;
		break;
	case 0x03:
		//BASE
		m_BASE = nCommand.nIMM;
		break;
	case 0x06:
		//MSKPATH3
		//Should mask bit somewhere...
		break;
	case 0x11:
		//FLUSH
		if(m_vpu.IsVuRunning())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
#ifdef DELAYED_MSCAL
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
#endif
		break;
	case 0x13:
		//FLUSHA
		if(m_vpu.IsVuRunning())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		break;
	case 0x50:
	case 0x51:
		//DIRECT/DIRECTHL
		Cmd_DIRECT(stream, nCommand);
		break;
	default:
		CVif::ExecuteCommand(stream, nCommand);
		break;
	}
}

void CVif1::Cmd_DIRECT(StreamType& stream, CODE nCommand)
{
	uint32 nSize = stream.GetAvailableReadBytes();
	nSize = std::min<uint32>(m_CODE.nIMM * 0x10, nSize);
	assert((nSize & 0x0F) == 0);

	if(nSize != 0)
	{
		if(m_directBuffer.size() < nSize)
		{
			m_directBuffer.resize(nSize);
		}

		auto packet = m_directBuffer.data();
		stream.Read(packet, nSize);

		int32 remainingLength = nSize;
		while(remainingLength > 0)
		{
			uint32 processed = m_gif.ProcessPacket(packet, 0, remainingLength, CGsPacketMetadata(2));
			packet += processed;
			remainingLength -= processed;
			assert(remainingLength >= 0);
		}
	}

	m_CODE.nIMM -= (nSize / 0x10);
	if((m_CODE.nIMM == 0) && (nSize != 0))
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
