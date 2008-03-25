#include "VPU.h"
#include "RegisterStateFile.h"

using namespace Framework;
using namespace boost;
using namespace std;

#define STATE_PREFIX            ("vif/vpu_")
#define STATE_SUFFIX            (".xml")
#define STATE_REGS_STAT         ("STAT")
#define STATE_REGS_CODE         ("CODE")
#define STATE_REGS_CYCLE        ("CYCLE")
#define STATE_REGS_NUM          ("NUM")

CVPU::CVPU(CVIF& vif, unsigned int vpuNumber, const CVIF::VPUINIT& vpuInit) :
m_vif(vif),
m_vpuNumber(vpuNumber),
m_pMicroMem(vpuInit.microMem),
m_pVUMem(vpuInit.vuMem),
m_pCtx(vpuInit.context),
m_execThread(NULL),
m_endThread(false),
m_executor(*vpuInit.context),
m_paused(true)
{
    m_execThread = new thread(bind(&CVPU::ExecuteThreadProc, this));
}

CVPU::~CVPU()
{
    if(m_execThread != NULL)
    {
        m_endThread = true;
        m_execCondition.notify_all();
        m_execThread->join();
        delete m_execThread;
    }
}

void CVPU::SingleStep()
{
    mutex::scoped_lock lock(m_execMutex);
    m_paused = false;
    m_execCondition.wait(m_execMutex);
}

void CVPU::Reset()
{
	memset(&m_STAT, 0, sizeof(STAT));
	memset(&m_CODE, 0, sizeof(CODE));
	memset(&m_CYCLE, 0, sizeof(CYCLE));
	m_NUM = 0;
}

void CVPU::SaveState(CZipArchiveWriter& archive)
{
    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
    registerFile->SetRegister32(STATE_REGS_STAT,    m_STAT);
    registerFile->SetRegister32(STATE_REGS_CODE,    m_CODE);
    registerFile->SetRegister32(STATE_REGS_CYCLE,   m_CYCLE);
    registerFile->SetRegister32(STATE_REGS_NUM,     m_NUM);
    archive.InsertFile(registerFile);
}

void CVPU::LoadState(CZipArchiveReader& archive)
{
    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
    m_STAT  = registerFile.GetRegister32(STATE_REGS_STAT);
    m_CODE  = registerFile.GetRegister32(STATE_REGS_CODE);
    m_CYCLE = registerFile.GetRegister32(STATE_REGS_CYCLE);
    m_NUM   = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_NUM));
}

uint32 CVPU::GetTOP()
{
    throw exception();
}

uint8* CVPU::GetVuMemory() const
{
    return m_pVUMem;
}

CMIPS& CVPU::GetContext() const
{
    return *m_pCtx;
}

void CVPU::ProcessPacket(uint32 nAddress, uint32 nSize)
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

uint32 CVPU::ExecuteCommand(CODE nCommand, uint32 nAddress, uint32 nSize)
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

void CVPU::ExecuteMicro(uint32 nAddress, uint32 nMask)
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
    m_paused = true;
    m_vif.SetStat(m_vif.GetStat() | nMask);

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

void CVPU::ExecuteThreadProc()
{
    while(!m_endThread)
    {
        if(!(m_vif.GetStat() & GetVbs()) || m_paused)
        {
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
			thread::sleep(xt);
        }
        else
        {
            m_pCtx->m_State.nHasException = 0;
            m_executor.Execute(1);
            m_paused = true;
            if(m_pCtx->m_State.nHasException)
            {
                //E bit encountered
                m_vif.SetStat(m_vif.GetStat() & ~GetVbs());
            }
#ifdef _DEBUG
            m_execCondition.notify_one();
#endif
        }
    }
}

uint32 CVPU::Cmd_MPG(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	uint32 nTransfered, nDstAddr, nNum, nCodeNum;

	nNum		= (m_NUM == 0) ? (256 * 8) : (m_NUM * 8);
	nCodeNum	= (m_CODE.nNUM == 0) ? (256 * 8) : (m_CODE.nNUM * 8);

	nSize = min(nNum, nSize);

	nTransfered = (nCodeNum - nNum) * 8;
	nDstAddr = (m_CODE.nIMM * 8) + nTransfered;

    //Check if microprogram is running
    if(m_vif.GetStat() & GetVbs())
    {
        throw exception();
    }

	//Check if there's a change
	if(memcmp(m_pMicroMem + nDstAddr, m_vif.GetRam() + nAddress, nSize) != 0)
	{
        m_executor.Clear();
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

uint32 CVPU::Cmd_UNPACK(CODE nCommand, uint32 nAddress, uint32 nSize)
{
	assert(0);
	return 0;
}

uint32 CVPU::Unpack_V45(uint32 nDstAddr, uint32 nSrcAddr, uint32 nSize)
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

uint32 CVPU::Unpack_V432(uint32 nDstAddr, uint32 nSrcAddr, uint32 nSize)
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

uint32 CVPU::GetVbs() const
{
    switch(m_vpuNumber)
    {
    case 0: 
        return CVIF::STAT_VBS0;
    case 1: 
        return CVIF::STAT_VBS1;
    default: 
        throw exception();
    }
}
