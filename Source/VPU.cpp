#include "VPU.h"
#include "Log.h"
#include "RegisterStateFile.h"
#include <boost/static_assert.hpp>
#include <boost/lexical_cast.hpp>

using namespace Framework;
using namespace boost;
using namespace std;

#define LOG_NAME                ("vpu")

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
    BOOST_STATIC_ASSERT(sizeof(CODE) == 4);

    m_execThread = new thread(bind(&CVPU::ExecuteThreadProc, this));
}

CVPU::~CVPU()
{
    if(m_execThread != NULL)
    {
        JoinThread();
    }
}

void CVPU::JoinThread()
{
    if(m_execThread != NULL)
    {
        m_endThread = true;
#ifdef _DEBUG
        m_execDoneCondition.notify_all();
#endif
        m_execThread->join();
        delete m_execThread;
        m_execThread = NULL;
    }
}

void CVPU::SingleStep()
{
    mutex::scoped_lock lock(m_execMutex);
    m_paused = false;
#ifdef _DEBUG
    m_execDoneCondition.wait(m_execMutex);
#endif
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

void CVPU::ProcessPacket(CVIF::CFifoStream& stream)
{
    while(stream.GetSize() != 0)
    {
        if(m_STAT.nVPS == 1)
        {
	        //Command is waiting for more data...
	        ExecuteCommand(m_CODE, stream);
	        continue;
        }

        stream.Read(&m_CODE, sizeof(CODE));

        assert(m_CODE.nI == 0);

        m_NUM = m_CODE.nNUM;

        ExecuteCommand(m_CODE, stream);
    }
}

uint32 CVPU::ExecuteCommand(CODE nCommand, CVIF::CFifoStream& stream)
{
	if(nCommand.nCMD >= 0x60)
	{
		return Cmd_UNPACK(nCommand, stream);
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
		return Cmd_MPG(nCommand, stream);
		break;
	default:
		assert(0);
		return 0;
		break;
	}
}

void CVPU::ExecuteMicro(uint32 nAddress)
{
#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_VU1ZONE);
#endif

    CLog::GetInstance().Print(LOG_NAME, "Starting microprogram execution at 0x%0.8X.\r\n", nAddress);

	m_pCtx->m_State.nPC = nAddress;
    m_paused = true;
    m_vif.SetStat(m_vif.GetStat() | GetVbs());
    m_execCondition.notify_all();
    m_STAT.nVEW = 0;

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif
}

void CVPU::ExecuteThreadProc()
{
    unsigned int quota = m_vif.IsVuDebuggingEnabled() ? 1 : 5000;
    while(!m_endThread)
    {
        if(
            !(m_vif.GetStat() & GetVbs()) || 
#ifdef _DEBUG
            (m_paused && m_vif.IsVuDebuggingEnabled())
#else
            false
#endif
            )
        {
            mutex::scoped_lock execLock(m_execMutex);
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 16 * 1000000;
            m_execCondition.timed_wait(m_execMutex, xt);
//			thread::sleep(xt);
//            thread::yield();
        }
        else
        {
            m_pCtx->m_State.nHasException = 0;
            m_executor.Execute(quota);
            if(m_pCtx->m_State.nHasException)
            {
                //E bit encountered
                m_vif.SetStat(m_vif.GetStat() & ~GetVbs());
            }
#ifdef _DEBUG
//            if(m_executor.MustBreak())
            {
                m_paused = true;
                m_execDoneCondition.notify_one();
            }
#endif
        }
    }
}

uint32 CVPU::Cmd_MPG(CODE nCommand, CVIF::CFifoStream& stream)
{
    uint32 nSize = stream.GetSize();

    uint32 nNum		= (m_NUM == 0) ? (256 * 8) : (m_NUM * 8);
    uint32 nCodeNum	= (m_CODE.nNUM == 0) ? (256 * 8) : (m_CODE.nNUM * 8);

    nSize = min(nNum, nSize);

    uint32 nTransfered = (nCodeNum - nNum) * 8;
    uint32 nDstAddr = (m_CODE.nIMM * 8) + nTransfered;

    //Check if microprogram is running
    if(m_vif.GetStat() & GetVbs())
    {
        throw exception();
    }

    uint8* microProgram = reinterpret_cast<uint8*>(alloca(nSize));
    stream.Read(microProgram, nSize);

    //Check if there's a change
    if(memcmp(m_pMicroMem + nDstAddr, microProgram, nSize) != 0)
    {
        m_executor.Clear();
        memcpy(m_pMicroMem + nDstAddr, microProgram, nSize);
    }

    m_NUM -= static_cast<uint8>(nSize / 8);
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

uint32 CVPU::Cmd_UNPACK(CODE nCommand, CVIF::CFifoStream& stream)
{
	assert(0);
	return 0;
}

void CVPU::Unpack_V45(uint32 nDstAddr, CVIF::CFifoStream& stream)
{
    assert(m_CYCLE.nCL == m_CYCLE.nWL);

    uint32 nSize = stream.GetSize();

    unsigned int nPackets = min<uint32>(nSize / 2, m_NUM);

    for(unsigned int i = 0; i < nPackets; i++)
    {
        uint16 nColor = 0;
        stream.Read(&nColor, 2);

	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x0]) = ((nColor >>  0) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x4]) = ((nColor >>  5) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0x8]) = ((nColor >> 10) & 0x1F) << 3;
	    *reinterpret_cast<uint32*>(&m_pVUMem[nDstAddr + 0xC]) = ((nColor >> 15) & 0x01) << 7;

	    nDstAddr += 0x10;

        m_NUM--;
    }

    //Force word alignment
    stream.Align32();
}

void CVPU::Unpack_V432(uint32 nDstAddr, CVIF::CFifoStream& stream)
{
    uint32 nCL = m_CYCLE.nCL;
    uint32 nWL = m_CYCLE.nWL;

    assert(nCL >= nWL);

    uint32 nSize = stream.GetSize();
    nSize = min(nSize, static_cast<uint32>(m_NUM) * 0x10);

    if(nCL == nWL)
    {
        stream.Read(m_pVUMem + nDstAddr, nSize);
        m_NUM -= static_cast<uint8>(nSize / 0x10);
    }
    else if(nCL > nWL)
    {
        for(unsigned int i = 0; i < nSize; i += 0x10)
        {
            uint32 nWritten = m_CODE.nNUM - m_NUM;
            uint32 nAddrInc = (nCL * (nWritten / nWL) + (nWritten % nWL)) * 0x10;

            stream.Read(m_pVUMem + nDstAddr + nAddrInc, 0x10);

            m_NUM -= 1;
        }
    }
    else
    {
        assert(0);
    }
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
