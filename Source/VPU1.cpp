#include "VPU1.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"

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

uint32 CVPU1::ExecuteCommand(CODE nCommand, uint32 nAddress, uint32 nSize)
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

        ExecuteMicro(nCommand.nIMM + PS2::VUMEM1SIZE, CVIF::STAT_VBS1);
		return 0;
		break;
	case 0x17:
		//MSCNT
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

        ExecuteMicro(m_pCtx->m_State.nPC, CVIF::STAT_VBS1);
		return 0;
		break;
	case 0x50:
	case 0x51:
		//DIRECT/DIRECTHL
		return Cmd_DIRECT(nCommand, nAddress, nSize);
		break;
	default:
		return CVPU::ExecuteCommand(nCommand, nAddress, nSize);
		break;
	}
}

uint32 CVPU1::Cmd_DIRECT(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	nSize = min(m_CODE.nIMM * 0x10, nSize);

	m_vif.GetGif().ProcessPacket(m_vif.GetRam(), nAddress, nAddress + nSize);

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

uint32 CVPU1::Cmd_UNPACK(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	bool nUsn, nFlg;
	uint32 nDstAddr, nTransfered;

	nFlg		= (m_CODE.nIMM & 0x8000) != 0;
	nUsn		= (m_CODE.nIMM & 0x4000) != 0;
	nDstAddr	= (m_CODE.nIMM & 0x03FF);

	if(nFlg) 
	{
		nDstAddr += m_TOPS;
	}

	nTransfered = m_CODE.nNUM - m_NUM;
	nDstAddr += nTransfered;

	nDstAddr *= 0x10;

	switch(nCommand.nCMD & 0x0F)
	{
	case 0x0C:
		//V4-32
		nSize = Unpack_V432(nDstAddr, nAddress, nSize);
		break;
	case 0x0F:
		//V4-5
		nSize = Unpack_V45(nDstAddr, nAddress, nSize);
		break;
	default:
		assert(0);
		nSize = 0;
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

	return nSize;
}
