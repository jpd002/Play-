#include <assert.h>
#include "VIF.h"
#include "GIF.h"
#include "Profiler.h"
#include "Ps2Const.h"

#ifdef	PROFILE
#define	PROFILE_VIFZONE "VIF"
#define PROFILE_VU1ZONE	"VU0"
#endif

using namespace Framework;
using namespace std;
using namespace boost;

//REMOVE
static int nExecTimes = 0;

CVIF::CVIF(CGIF& gif, uint8* ram, const VPUINIT& vpu0Init, const VPUINIT& vpu1Init) :
m_gif(gif),
m_ram(ram),
m_pauseNeeded(false)
{
    m_pVPU[0] = new CVPU(*this, vpu0Init);
    m_pVPU[1] = new CVPU1(*this, vpu1Init);
}

CVIF::~CVIF()
{
    for(unsigned int i = 0; i < 2; i++)
    {
        delete m_pVPU[i];
    }
}

void CVIF::Reset()
{
    m_pVPU[0]->Reset();
    m_pVPU[1]->Reset();
    m_VPU_STAT = 0;
}

void CVIF::SaveState(CStream* pStream)
{
    pStream->Write(&m_VPU_STAT,     4);

    m_pVPU[0]->SaveState(pStream);
    m_pVPU[1]->SaveState(pStream);
}

void CVIF::LoadState(CStream* pStream)
{
	pStream->Read(&m_VPU_STAT,  4);

	m_pVPU[0]->LoadState(pStream);
	m_pVPU[1]->LoadState(pStream);
}

uint32 CVIF::ReceiveDMA1(uint32 nAddress, uint32 nQWC, bool nTagIncluded)
{

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_VIFZONE);
#endif

	if(nTagIncluded)
	{
		m_pVPU[1]->ProcessPacket(nAddress + 8, (nQWC * 0x10) - 8);
	}
	else
	{
		m_pVPU[1]->ProcessPacket(nAddress, nQWC * 0x10);
	}
	
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

	return nQWC;
}

uint32* CVIF::GetTop1Address()
{
	return m_pVPU[1]->GetTOP();
}

void CVIF::StopVU(CMIPS* pCtx)
{
    if(pCtx == &m_pVPU[1]->GetContext())
    {
		m_VPU_STAT &= ~STAT_VBS1;
    }
}

void CVIF::ProcessXGKICK(uint32 nAddress)
{
	nAddress &= 0xFFFF;
	nAddress *= 0x10;

	m_gif.ProcessPacket(m_pVPU[1]->GetVuMemory(), nAddress, PS2::VUMEM1SIZE);
}

bool CVIF::IsVU1Running()
{
	return (m_VPU_STAT & STAT_VBS1) != 0;
}

bool CVIF::IsPauseNeeded() const
{
    return m_pauseNeeded;
}

void CVIF::SetPauseNeeded(bool value)
{
    m_pauseNeeded = value;
}

uint8* CVIF::GetRam() const
{
    return m_ram;
}

CGIF& CVIF::GetGif() const
{
    return m_gif;
}

uint32 CVIF::GetStat() const
{
    return m_VPU_STAT;
}

void CVIF::SetStat(uint32 stat)
{
    m_VPU_STAT = stat;
}

////////////////////////////////////////////////////////////
// VPU Class Implementation

CVIF::CVPU::CVPU(CVIF& vif, const VPUINIT& vpuInit) :
m_vif(vif),
m_pMicroMem(vpuInit.microMem),
m_pVUMem(vpuInit.vuMem),
m_pCtx(vpuInit.context),
m_execThread(NULL),
m_endThread(false)
{
//    m_execThread = new thread(bind(&CVPU::ExecuteThreadProc, this));
}

CVIF::CVPU::~CVPU()
{
    if(m_execThread != NULL)
    {
        m_endThread = true;
        m_execCondition.notify_all();
        m_execThread->join();
        delete m_execThread;
    }
}

void CVIF::CVPU::Reset()
{
	memset(&m_STAT, 0, sizeof(STAT));
	memset(&m_CODE, 0, sizeof(CODE));
	memset(&m_CYCLE, 0, sizeof(CYCLE));
	m_NUM = 0;
}

void CVIF::CVPU::SaveState(CStream* pStream)
{
	pStream->Write(&m_STAT,		sizeof(STAT));
	pStream->Write(&m_CODE,		sizeof(CODE));
	pStream->Write(&m_CYCLE,	sizeof(CYCLE));
	pStream->Write(&m_NUM,		1);
}

void CVIF::CVPU::LoadState(CStream* pStream)
{
	pStream->Read(&m_STAT,		sizeof(STAT));
	pStream->Read(&m_CODE,		sizeof(CODE));
	pStream->Read(&m_CYCLE,		sizeof(CYCLE));
	pStream->Read(&m_NUM,		1);
}

uint32* CVIF::CVPU::GetTOP()
{
	assert(0);
	return NULL;
}

uint8* CVIF::CVPU::GetVuMemory() const
{
    return m_pVUMem;
}

CMIPS& CVIF::CVPU::GetContext() const
{
    return *m_pCtx;
}

void CVIF::CVPU::ProcessPacket(uint32 nAddress, uint32 nSize)
{
    uint32 nEnd = nAddress + nSize;

    while(nAddress < nEnd)
    {
        if(m_STAT.nVPS == 1)
        {
	        //Command is waiting for more data...
	        nAddress += ExecuteCommand(m_CODE, nAddress, (nEnd - nAddress));
	        continue;
        }

        CODE Code = *reinterpret_cast<CODE*>(m_vif.GetRam() + nAddress);

        nAddress += 4;

        assert(Code.nI == 0);

        m_NUM = Code.nNUM;
        m_CODE = Code;

        nAddress += ExecuteCommand(Code, nAddress, (nEnd - nAddress));
    }
}

uint32 CVIF::CVPU::ExecuteCommand(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	if(nCommand.nCMD >= 0x60)
	{
		return Cmd_UNPACK(nCommand, nAddress, nSize);
	}
	switch(nCommand.nCMD)
	{
	case 0:
		//NOP
		return 0;
		break;
	case 0x01:
		//STCYCL
		m_CYCLE = nCommand.nIMM;
		return 0;
		break;
	case 0x14:
		//MSCAL
		assert(0);
		return 0;
		break;
	case 0x17:
		//MSCNT
		assert(0);
		return 0;
		break;
	case 0x4A:
		//MPG
		return Cmd_MPG(nCommand, nAddress, nSize);
		break;
	default:
		assert(0);
		return 0;
		break;
	}
}

void CVIF::CVPU::ExecuteMicro(uint32 nAddress, uint32 nMask)
{
	//REMOVE
/*
	nExecTimes++;
	if(nExecTimes > 0x7E)
	{
		if(!CPS2VM::m_EE.MustBreak())
		{
			CPS2VM::m_EE.ToggleBreakpoint(CPS2VM::m_EE.m_State.nPC);
		}
		return;
	}
*/

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_VU1ZONE);
#endif

	m_pCtx->m_State.nPC = nAddress;
    m_vif.SetStat(m_vif.GetStat() | nMask);
    m_vif.SetPauseNeeded(true);

#ifndef VU_DEBUG

//	while(m_vif.GetStat() & nMask)
//	{
//		RET_CODE nRet;
//		nRet = m_pCtx->Execute(100000);
//	}

#endif

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

}

uint32 CVIF::CVPU::Cmd_MPG(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	uint32 nTransfered, nDstAddr, nNum, nCodeNum;

	nNum		= (m_NUM == 0) ? (256 * 8) : (m_NUM * 8);
	nCodeNum	= (m_CODE.nNUM == 0) ? (256 * 8) : (m_CODE.nNUM * 8);

	nSize = min(nNum, nSize);

	nTransfered = (nCodeNum - nNum) * 8;
	nDstAddr = (m_CODE.nIMM * 8) + nTransfered;

	//Check if there's a change
	if(memcmp(m_pMicroMem + nDstAddr, m_vif.GetRam() + nAddress, nSize) != 0)
	{
//		m_pCtx->m_pExecMap->InvalidateBlocks();
        memcpy(m_pMicroMem + nDstAddr, m_vif.GetRam() + nAddress, nSize);
	}

	m_NUM -= (uint8)(nSize / 8);
	if((m_NUM == 0) && (nSize != 0))
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}

	return nSize;
}

uint32 CVIF::CVPU::Cmd_UNPACK(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	assert(0);
	return 0;
}

uint32 CVIF::CVPU::Unpack_V45(uint32 nDstAddr, uint32 nSrcAddr, uint32 nSize)
{
    assert(m_CYCLE.nCL == m_CYCLE.nWL);

    unsigned int nPackets = min(nSize / 2, m_NUM);
    uint32 nAddress = nSrcAddr;

    for(unsigned int i = 0; i < nPackets; i++)
    {
	    uint16 nColor = *reinterpret_cast<uint16*>(m_vif.GetRam() + nAddress);

	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x0]) = ((nColor >>  0) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x4]) = ((nColor >>  5) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x8]) = ((nColor >> 10) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0xC]) = ((nColor >> 15) & 0x01) << 7;

	    nDstAddr += 0x10;
	    nAddress += 0x02;

	    m_NUM--;
    }

    //Force word alignment
    if(nAddress & 0x03)
    {
        nAddress += 0x02;
    }

    return nAddress - nSrcAddr;
}

uint32 CVIF::CVPU::Unpack_V432(uint32 nDstAddr, uint32 nSrcAddr, uint32 nSize)
{
	uint32 nCL, nWL;

	nCL = m_CYCLE.nCL;
	nWL = m_CYCLE.nWL;

	assert(nCL >= nWL);

	nSize = min(nSize, (uint32)m_NUM * 0x10);

	if(nCL == nWL)
	{
		memcpy(m_pVUMem + nDstAddr, m_vif.GetRam() + nSrcAddr, nSize);
		m_NUM -= (uint8)(nSize / 0x10);
	}
	else if(nCL > nWL)
	{
		uint32 nWritten, nAddrInc;

		for(unsigned int i = 0; i < nSize; i += 0x10)
		{
			nWritten = m_CODE.nNUM - m_NUM;
			nAddrInc = (nCL * (nWritten / nWL) + (nWritten % nWL)) * 0x10;
			
			memcpy(m_pVUMem + nDstAddr + nAddrInc, m_vif.GetRam() + nSrcAddr, 0x10);

			nSrcAddr += 0x10;
			m_NUM -= 1;
		}
	}
	else
	{
		assert(0);
	}

	return nSize;
}

////////////////////////////////////////////////////////////
// VPU1 Class Implementation

CVIF::CVPU1::CVPU1(CVIF& vif, const VPUINIT& vpuInit) :
CVPU(vif, vpuInit)
{

}

void CVIF::CVPU1::Reset()
{
	CVPU::Reset();
	m_BASE	= 0;
	m_TOP	= 0;
	m_TOPS	= 0;
	m_OFST	= 0;
}

void CVIF::CVPU1::SaveState(CStream* pStream)
{
	CVPU::SaveState(pStream);

	pStream->Write(&m_BASE,		4);
	pStream->Write(&m_TOP,		4);
	pStream->Write(&m_TOPS,		4);
	pStream->Write(&m_OFST,		4);
}

void CVIF::CVPU1::LoadState(CStream* pStream)
{
	CVPU::LoadState(pStream);

	pStream->Read(&m_BASE,		4);
	pStream->Read(&m_TOP,		4);
	pStream->Read(&m_TOPS,		4);
	pStream->Read(&m_OFST,		4);
}

uint32* CVIF::CVPU1::GetTOP()
{
	return &m_TOP;
}

uint32 CVIF::CVPU1::ExecuteCommand(CODE nCommand, uint32 nAddress, uint32 nSize)
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
		return 0;
		break;
	case 0x13:
		//FLUSHA
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

		ExecuteMicro(nCommand.nIMM + PS2::VUMEM1SIZE, STAT_VBS1);
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

		ExecuteMicro(m_pCtx->m_State.nPC, STAT_VBS1);
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

uint32 CVIF::CVPU1::Cmd_DIRECT(CODE nCommand, uint32 nAddress, uint32 nSize)
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

uint32 CVIF::CVPU1::Cmd_UNPACK(CODE nCommand, uint32 nAddress, uint32 nSize)
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
/*
	if(nFlg == 1 && nUnit == 1)
	{
		nImm += (uint16)m_VIF1_TOPS;
	}

	nImm *= 0x10;
	nNum *= 0x10;

	nWL = (uint8)(((*pCycle) & 0xFF00) >> 8);
	nCL = (uint8)(((*pCycle) & 0x00FF) >> 0);

	//Check format
	switch(nCmd & 0x0F)
	{
	case 0x0C:
		//V4-32
		nAddress += UnpackV432(nAddress, nImm, nNum, nUsn, nWL, nCL, pVUMemory);
		break;
	case 0x0F:
		//V4-5
		nAddress += UnpackV45(nAddress, nImm, nNum, nUsn, nWL, nCL, pVUMemory);
		break;
	default:
		assert(0);
		break;
	}
*/
}
