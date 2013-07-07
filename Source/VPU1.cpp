#include "VPU1.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"
#include "FrameDump.h"
#include <boost/lexical_cast.hpp>

#define STATE_PREFIX			("vif/vpu1_")
#define STATE_SUFFIX			(".xml")
#define STATE_REGS_BASE			("BASE")
#define STATE_REGS_TOP			("TOP")
#define STATE_REGS_TOPS			("TOPS")
#define STATE_REGS_OFST			("OFST")

CVPU1::CVPU1(CVIF& vif, unsigned int vpuNumber, const CVIF::VPUINIT& vpuInit) 
: CVPU(vif, vpuNumber, vpuInit)
{

}

void CVPU1::Reset()
{
	CVPU::Reset();
	m_BASE	= 0;
	m_TOP	= 0;
	m_TOPS	= 0;
	m_OFST	= 0;
}

void CVPU1::SaveState(Framework::CZipArchiveWriter& archive)
{
	CVPU::SaveState(archive);

	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_vpuNumber) + STATE_SUFFIX;
	CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
	registerFile->SetRegister32(STATE_REGS_BASE,	m_BASE);
	registerFile->SetRegister32(STATE_REGS_TOP,		m_TOP);
	registerFile->SetRegister32(STATE_REGS_TOPS,	m_TOPS);
	registerFile->SetRegister32(STATE_REGS_OFST,	m_OFST);
	archive.InsertFile(registerFile);
}

void CVPU1::LoadState(Framework::CZipArchiveReader& archive)
{
	CVPU::LoadState(archive);

	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_vpuNumber) + STATE_SUFFIX;
	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_BASE	= registerFile.GetRegister32(STATE_REGS_BASE);
	m_TOP	= registerFile.GetRegister32(STATE_REGS_TOP);
	m_TOPS	= registerFile.GetRegister32(STATE_REGS_TOPS);
	m_OFST	= registerFile.GetRegister32(STATE_REGS_OFST);
}

uint32 CVPU1::GetTOP() const
{
	return m_TOP;
}

void CVPU1::ExecuteCommand(StreamType& stream, CODE nCommand)
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
//		return 0;
		break;
	case 0x03:
		//BASE
		m_BASE = nCommand.nIMM;
//		return 0;
		break;
	case 0x06:
		//MSKPATH3
		//Should mask bit somewhere...
//		return 0;
		break;
	case 0x11:
		//FLUSH
		if(m_vif.IsVu1Running())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		break;
	case 0x13:
		//FLUSHA
		if(m_vif.IsVu1Running())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
//		return 0;
		break;
	case 0x50:
	case 0x51:
		//DIRECT/DIRECTHL
		return Cmd_DIRECT(stream, nCommand);
		break;
	default:
		return CVPU::ExecuteCommand(stream, nCommand);
		break;
	}
}

void CVPU1::Cmd_DIRECT(StreamType& stream, CODE nCommand)
{
	uint32 nSize = stream.GetAvailableReadBytes();
	nSize = std::min<uint32>(m_CODE.nIMM * 0x10, nSize);
	assert((nSize & 0x0F) == 0);

	if(nSize != 0)
	{
		uint8* packet = reinterpret_cast<uint8*>(alloca(nSize));
		stream.Read(packet, nSize);

		int32 remainingLength = nSize;
		while(remainingLength > 0)
		{
			uint32 processed = m_vif.GetGif().ProcessPacket(packet, 0, remainingLength, CGsPacketMetadata(2));
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

void CVPU1::Cmd_UNPACK(StreamType& stream, CODE nCommand, uint32 nDstAddr)
{
	bool nFlg = (m_CODE.nIMM & 0x8000) != 0;
	if(nFlg) 
	{
		nDstAddr += m_TOPS;
	}

	return CVPU::Cmd_UNPACK(stream, nCommand, nDstAddr);
}

void CVPU1::StartMicroProgram(uint32 address)
{
	if(IsRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	assert(!m_STAT.nVEW);

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

	ExecuteMicro(address);
}
