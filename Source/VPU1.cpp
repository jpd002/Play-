#include "VPU1.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"
#include <boost/lexical_cast.hpp>

#define STATE_PREFIX         ("vif/vpu1_")
#define STATE_SUFFIX         (".xml")
#define STATE_REGS_BASE      ("BASE")
#define STATE_REGS_TOP       ("TOP")
#define STATE_REGS_TOPS      ("TOPS")
#define STATE_REGS_OFST      ("OFST")

using namespace std;
using namespace boost;
using namespace Framework;

CVPU1::CVPU1(CVIF& vif, unsigned int vpuNumber, const CVIF::VPUINIT& vpuInit) :
CVPU(vif, vpuNumber, vpuInit)
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

void CVPU1::SaveState(CZipArchiveWriter& archive)
{
	CVPU::SaveState(archive);

    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
    registerFile->SetRegister32(STATE_REGS_BASE,    m_BASE);
    registerFile->SetRegister32(STATE_REGS_TOP,     m_TOP);
    registerFile->SetRegister32(STATE_REGS_TOPS,    m_TOPS);
    registerFile->SetRegister32(STATE_REGS_OFST,    m_OFST);
    archive.InsertFile(registerFile);
}

void CVPU1::LoadState(CZipArchiveReader& archive)
{
    CVPU::LoadState(archive);

    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
    m_BASE  = registerFile.GetRegister32(STATE_REGS_BASE);
    m_TOP   = registerFile.GetRegister32(STATE_REGS_TOP);
    m_TOPS  = registerFile.GetRegister32(STATE_REGS_TOPS);
    m_OFST  = registerFile.GetRegister32(STATE_REGS_OFST);
}

uint32 CVPU1::GetTOP()
{
	return m_TOP;
}

uint32 CVPU1::ExecuteCommand(CODE nCommand, CVIF::CFifoStream& stream)
{
	switch(nCommand.nCMD)
	{
	case 0x02:
		//OFFSET
		m_OFST = nCommand.nIMM;
		m_STAT.nDBF = 0;
		m_TOPS = m_BASE;
		return 0;
		break;
	case 0x03:
		//BASE
		m_BASE = nCommand.nIMM;
		return 0;
		break;
	case 0x06:
		//MSKPATH3
		//Should mask bit somewhere...
		return 0;
		break;
	case 0x11:
		//FLUSH
        assert(!m_vif.IsVU1Running());
		return 0;
		break;
	case 0x13:
		//FLUSHA
        assert(!m_vif.IsVU1Running());
		return 0;
		break;
	case 0x14:
		//MSCAL
        StartMicroProgram(nCommand.nIMM + PS2::VUMEM1SIZE);
		return 0;
		break;
	case 0x17:
		//MSCNT
        StartMicroProgram(m_pCtx->m_State.nPC);
		return 0;
		break;
	case 0x50:
	case 0x51:
		//DIRECT/DIRECTHL
		return Cmd_DIRECT(nCommand, stream);
		break;
	default:
		return CVPU::ExecuteCommand(nCommand, stream);
		break;
	}
}

uint32 CVPU1::Cmd_DIRECT(CODE nCommand, CVIF::CFifoStream& stream)
{
    uint32 nSize = stream.GetSize();
    nSize = min<uint32>(m_CODE.nIMM * 0x10, nSize);

    uint8* packet = reinterpret_cast<uint8*>(alloca(nSize));
    stream.Read(packet, nSize);

    m_vif.GetGif().ProcessPacket(packet, 0, nSize);

    m_CODE.nIMM -= (nSize / 0x10);
    if((m_CODE.nIMM == 0) && (nSize != 0))
    {
        m_STAT.nVPS = 0;
    }
    else
    {
        m_STAT.nVPS = 1;
    }

    return nSize;
}

uint32 CVPU1::Cmd_UNPACK(CODE nCommand, CVIF::CFifoStream& stream)
{
    bool nFlg       = (m_CODE.nIMM & 0x8000) != 0;
    bool nUsn       = (m_CODE.nIMM & 0x4000) != 0;
    uint32 nDstAddr = (m_CODE.nIMM & 0x03FF);

    if(nFlg) 
    {
        nDstAddr += m_TOPS;
    }

    uint32 nTransfered = m_CODE.nNUM - m_NUM;
    nDstAddr += nTransfered;

    nDstAddr *= 0x10;

    switch(nCommand.nCMD & 0x0F)
    {
    case 0x0C:
        //V4-32
        Unpack_V432(nDstAddr, stream);
        break;
    case 0x0F:
        //V4-5
        Unpack_V45(nDstAddr, stream);
        break;
    default:
        assert(0);
        break;
    }

    if(m_NUM != 0)
    {
        m_STAT.nVPS = 1;
    }
    else
    {
        m_STAT.nVPS = 0;
    }

    return 0;
}

void CVPU1::StartMicroProgram(uint32 address)
{
    if(m_vif.IsVU1Running())
    {
        m_STAT.nVEW = 1;
        return;
    }

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
