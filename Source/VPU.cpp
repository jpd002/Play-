#include "VPU.h"
#include "Log.h"
#include "RegisterStateFile.h"
#include <boost/lexical_cast.hpp>

using namespace Framework;
using namespace boost;
using namespace std;
using namespace std::tr1;

#define LOG_NAME                ("vpu")

#define STATE_PREFIX            ("vif/vpu_")
#define STATE_SUFFIX            (".xml")
#define STATE_REGS_STAT         ("STAT")
#define STATE_REGS_CODE         ("CODE")
#define STATE_REGS_CYCLE        ("CYCLE")
#define STATE_REGS_NUM          ("NUM")
#define STATE_REGS_MASK         ("MASK")
#define STATE_REGS_MODE         ("MODE")
#define STATE_REGS_ROW0         ("ROW0")
#define STATE_REGS_ROW1         ("ROW1")
#define STATE_REGS_ROW2         ("ROW2")
#define STATE_REGS_ROW3         ("ROW3")
#define STATE_REGS_ITOP         ("ITOP")
#define STATE_REGS_ITOPS        ("ITOPS")
#define STATE_REGS_READTICK     ("readTick")
#define STATE_REGS_WRITETICK    ("writeTick")

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
    m_executor.Clear();
	memset(&m_STAT, 0, sizeof(STAT));
	memset(&m_CODE, 0, sizeof(CODE));
	memset(&m_CYCLE, 0, sizeof(CYCLE));
    memset(&m_R, 0, sizeof(m_R));
    m_MODE = 0;
	m_NUM = 0;
    m_MASK = 0;
    m_ITOP = 0;
    m_ITOPS = 0;
}

void CVPU::SaveState(CZipArchiveWriter& archive)
{
    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
    registerFile->SetRegister32(STATE_REGS_STAT,        m_STAT);
    registerFile->SetRegister32(STATE_REGS_CODE,        m_CODE);
    registerFile->SetRegister32(STATE_REGS_CYCLE,       m_CYCLE);
    registerFile->SetRegister32(STATE_REGS_NUM,         m_NUM);
    registerFile->SetRegister32(STATE_REGS_MODE,        m_MODE);
    registerFile->SetRegister32(STATE_REGS_MASK,        m_MASK);
    registerFile->SetRegister32(STATE_REGS_ROW0,        m_R[0]);
    registerFile->SetRegister32(STATE_REGS_ROW1,        m_R[1]);
    registerFile->SetRegister32(STATE_REGS_ROW2,        m_R[2]);
    registerFile->SetRegister32(STATE_REGS_ROW3,        m_R[3]);
    registerFile->SetRegister32(STATE_REGS_ITOP,        m_ITOP);
    registerFile->SetRegister32(STATE_REGS_ITOPS,       m_ITOPS);
    registerFile->SetRegister32(STATE_REGS_READTICK,    m_readTick);
    registerFile->SetRegister32(STATE_REGS_WRITETICK,   m_writeTick);
    archive.InsertFile(registerFile);
}

void CVPU::LoadState(CZipArchiveReader& archive)
{
    string path = STATE_PREFIX + lexical_cast<string>(m_vpuNumber) + STATE_SUFFIX;
    CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
    m_STAT    <<= registerFile.GetRegister32(STATE_REGS_STAT);
    m_CODE    <<= registerFile.GetRegister32(STATE_REGS_CODE);
    m_CYCLE   <<= registerFile.GetRegister32(STATE_REGS_CYCLE);
    m_NUM       = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_NUM));
    m_MODE      = registerFile.GetRegister32(STATE_REGS_MODE);
    m_MASK      = registerFile.GetRegister32(STATE_REGS_MASK);
    m_R[0]      = registerFile.GetRegister32(STATE_REGS_ROW0);
    m_R[1]      = registerFile.GetRegister32(STATE_REGS_ROW1);
    m_R[2]      = registerFile.GetRegister32(STATE_REGS_ROW2);
    m_R[3]      = registerFile.GetRegister32(STATE_REGS_ROW3);
    m_ITOP      = registerFile.GetRegister32(STATE_REGS_ITOP);
    m_ITOPS     = registerFile.GetRegister32(STATE_REGS_ITOPS);
    m_readTick  = registerFile.GetRegister32(STATE_REGS_READTICK);
    m_writeTick = registerFile.GetRegister32(STATE_REGS_WRITETICK);
}

uint32 CVPU::GetTOP() const
{
    throw exception();
}

uint32 CVPU::GetITOP() const
{
    return m_ITOP;
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
        if(m_STAT.nVEW == 1)
        {
            //Command is waiting for micro-program to end.
            ExecuteCommand(m_CODE, stream);
            if(m_STAT.nVEW == 1)
            {
                break;
            }
        }

        stream.Read(&m_CODE, sizeof(CODE));

        assert(m_CODE.nI == 0);

        m_NUM = m_CODE.nNUM;

        ExecuteCommand(m_CODE, stream);
    }
}

void CVPU::ExecuteCommand(CODE nCommand, CVIF::CFifoStream& stream)
{
#ifdef _DEBUG
    if(m_vpuNumber == 0)
    {
        DisassembleCommand(nCommand);
    }
#endif
	if(nCommand.nCMD >= 0x60)
	{
		return Cmd_UNPACK(nCommand, stream, (nCommand.nIMM & 0x03FF));
	}
	switch(nCommand.nCMD)
	{
	case 0:
		//NOP
		break;
	case 0x01:
		//STCYCL
		m_CYCLE <<= nCommand.nIMM;
		break;
    case 0x04:
        m_ITOPS = nCommand.nIMM & 0x3FF;
        break;
    case 0x05:
        //STMOD
        m_MODE = nCommand.nIMM & 0x03;
        break;
	case 0x14:
		//MSCAL
        StartMicroProgram(nCommand.nIMM * 8);
		break;
	case 0x17:
		//MSCNT
        StartMicroProgram(m_pCtx->m_State.nPC);
		break;
    case 0x20:
        //STMASK
        return Cmd_STMASK(nCommand, stream);
        break;
    case 0x30:
        //STROW
        return Cmd_STROW(nCommand, stream);
        break;
	case 0x4A:
		//MPG
		return Cmd_MPG(nCommand, stream);
		break;
	default:
		assert(0);
		break;
	}
}

void CVPU::StartMicroProgram(uint32 address)
{
    if(IsRunning())
    {
        m_STAT.nVEW = 1;
        return;
    }

    assert(!m_STAT.nVEW);

    ExecuteMicro(address);
}

void CVPU::ExecuteMicro(uint32 nAddress)
{
#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_VU1ZONE);
#endif

    m_ITOP = m_ITOPS;

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
    bool isDebuggingEnabled = m_vpuNumber == 0 ? m_vif.IsVu0DebuggingEnabled() : m_vif.IsVu1DebuggingEnabled();
    unsigned int quota = isDebuggingEnabled ? 1 : 5000;
    while(!m_endThread)
    {
        if(
            !(m_vif.GetStat() & GetVbs()) || 
#ifdef _DEBUG
            (m_paused && isDebuggingEnabled)
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

void CVPU::Cmd_MPG(CODE nCommand, CVIF::CFifoStream& stream)
{
    uint32 nSize = stream.GetSize();

    uint32 nNum         = (m_NUM == 0) ? (256) : (m_NUM);
    uint32 nCodeNum     = (m_CODE.nNUM == 0) ? (256) : (m_CODE.nNUM);
    uint32 nTransfered  = (nCodeNum - nNum) * 8;

    nCodeNum *= 8;
    nNum *= 8;

    nSize = min(nNum, nSize);

    uint32 nDstAddr = (m_CODE.nIMM * 8) + nTransfered;

    //Check if microprogram is running
    if(IsRunning())
    {
        m_STAT.nVEW = 1;
        return;
    }

    if(nSize != 0)
    {
        uint8* microProgram = reinterpret_cast<uint8*>(alloca(nSize));
        stream.Read(microProgram, nSize);

        //Check if there's a change
        if(memcmp(m_pMicroMem + nDstAddr, microProgram, nSize) != 0)
        {
            m_executor.Clear();
            memcpy(m_pMicroMem + nDstAddr, microProgram, nSize);
        }
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

//    return nSize;
}

void CVPU::Cmd_STROW(CODE nCommand, CVIF::CFifoStream& stream)
{
    if(m_NUM == 0)
    {
        m_NUM = 4;
    }

//    uint32 transfered = 0;
    while(m_NUM != 0 && stream.GetSize())
    {
        assert(m_NUM <= 4);
        stream.Read(&m_R[4 - m_NUM], 4);
        m_NUM--;
//        transfered += 4;
    }

    if(m_NUM == 0)
    {
        m_STAT.nVPS = 0;
    }
    else
    {
        m_STAT.nVPS = 1;
    }

//    return transfered;
}

void CVPU::Cmd_STMASK(CODE command, CVIF::CFifoStream& stream)
{
    if(m_NUM == 0)
    {
        m_NUM = 1;
    }

    while(m_NUM != 0 && stream.GetSize())
    {
        stream.Read(&m_MASK, 4);
        m_NUM--;
    }

    if(m_NUM == 0)
    {
        m_STAT.nVPS = 0;
    }
    else
    {
        m_STAT.nVPS = 1;
    }
}

void CVPU::Cmd_UNPACK(CODE nCommand, CVIF::CFifoStream& stream, uint32 nDstAddr)
{
    assert((nCommand.nCMD & 0x60) == 0x60);

    bool usn = (m_CODE.nIMM & 0x4000) != 0;
    bool useMask = (nCommand.nCMD & 0x10) != 0;
    uint32 cl = m_CYCLE.nCL;
    uint32 wl = m_CYCLE.nWL;

    if(m_NUM == nCommand.nNUM)
    {
        m_readTick = 0;
        m_writeTick = 0;
    }

    assert(m_NUM == nCommand.nNUM);
//    uint32 nTransfered = m_CODE.nNUM - m_NUM;
//    nDstAddr += nTransfered;

    nDstAddr *= 0x10;

    uint128* dst = reinterpret_cast<uint128*>(&m_pVUMem[nDstAddr]);

    while(m_NUM != 0 && stream.GetSize())
    {
        bool mustWrite = false;
        uint128 writeValue;

        if(cl >= wl)
        {
            if(m_writeTick != wl || wl == 0)
            {
                bool success = false;
                switch(nCommand.nCMD & 0x0F)
                {
                case 0x00:
                    //S-32
                    success = Unpack_S32(stream, writeValue);
                    break;
                case 0x01:
                    //S-16
                    success = Unpack_S16(stream, writeValue, usn);
                    break;
                case 0x05:
                    //V2-16
                    success = Unpack_V16(stream, writeValue, 2, usn);
                    break;
                case 0x08:
                    //V3-32
                    success = Unpack_V32(stream, writeValue, 3);
                    break;
                case 0x09:
                    //V3-16
                    success = Unpack_V16(stream, writeValue, 3, usn);
                    break;
                case 0x0C:
                    //V4-32
                    success = Unpack_V32(stream, writeValue, 4);
                    break;
                case 0x0D:
                    //V4-16
                    success = Unpack_V16(stream, writeValue, 4, usn);
                    break;
                case 0x0E:
                    //V4-8
                    success = Unpack_V8(stream, writeValue, 4, usn);
                    break;
                case 0x0F:
                    //V4-5
                    success = Unpack_V45(stream, writeValue);
                    break;
                default:
                    assert(0);
                    break;
                }

                if(!success) break;
                mustWrite = true;
            }
        }
        else
        {
            assert(0);
        }

        if(mustWrite)
        {
            for(unsigned int i = 0; i < 4; i++)
            {
                uint32 maskOp = useMask ? GetMaskOp(i, m_writeTick) : MASK_DATA;

                if(maskOp == MASK_DATA)
                {
                    if(m_MODE == MODE_OFFSET)
                    {
                        writeValue.nV[i] += m_R[i];
                    }
                    else if(m_MODE == MODE_DIFFERENCE)
                    {
                        assert(0);
                    }

                    dst->nV[i] = writeValue.nV[i];
                }
                else if(maskOp == MASK_MASK)
                {

                }
                else
                {
                    assert(0);
                }
            }
            
            m_NUM--;
        }

        m_writeTick = min<uint32>(m_writeTick + 1, wl);
        m_readTick = min<uint32>(m_readTick + 1, cl);

        if(m_writeTick == wl && m_readTick == cl)
        {
            m_writeTick = 0;
            m_readTick = 0;
        }

        dst++;
    }

    if(m_NUM != 0)
    {
        m_STAT.nVPS = 1;
    }
    else
    {
        stream.Align32();
        m_STAT.nVPS = 0;
    }

//    return 0;
}

bool CVPU::Unpack_S32(CVIF::CFifoStream& stream, uint128& result)
{
    if(stream.GetSize() < 4) return false;

    uint32 word = 0;
    stream.Read(&word, 4);

    for(unsigned int i = 0; i < 4; i++)
    {
        result.nV[i] = word;
    }

    return true;
}

bool CVPU::Unpack_S16(CVIF::CFifoStream& stream, uint128& result, bool zeroExtend)
{
    if(stream.GetSize() < 2) return false;

    uint32 temp = 0;
    stream.Read(&temp, 2);
    if(!zeroExtend)
    {
        temp = static_cast<int16>(temp);
    }

    for(unsigned int i = 0; i < 4; i++)
    {
        result.nV[i] = temp;
    }

    return true;
}

bool CVPU::Unpack_V8(CVIF::CFifoStream& stream, uint128& result, unsigned int fields, bool zeroExtend)
{
    if(stream.GetSize() < (fields)) return false;

    for(unsigned int i = 0; i < fields; i++)
    {
        uint32 temp = 0;
        stream.Read(&temp, 1);
        if(!zeroExtend)
        {
            temp = static_cast<int8>(temp);
        }

        result.nV[i] = temp;
    }

    return true;
}

bool CVPU::Unpack_V16(CVIF::CFifoStream& stream, uint128& result, unsigned int fields, bool zeroExtend)
{
    if(stream.GetSize() < (fields * 2)) return false;

    for(unsigned int i = 0; i < fields; i++)
    {
        uint32 temp = 0;
        stream.Read(&temp, 2);
        if(!zeroExtend)
        {
            temp = static_cast<int16>(temp);
        }

        result.nV[i] = temp;
    }

    return true;
}

bool CVPU::Unpack_V32(CVIF::CFifoStream& stream, uint128& result, unsigned int fields)
{
    if(stream.GetSize() < (fields * 4)) return false;

    stream.Read(&result, (fields * 4));

    return true;
}

bool CVPU::Unpack_V45(CVIF::CFifoStream& stream, uint128& result)
{
    if(stream.GetSize() < 2) return false;

    uint16 nColor = 0;
    stream.Read(&nColor, 2);

    result.nV0 = ((nColor >>  0) & 0x1F) << 3;
    result.nV1 = ((nColor >>  5) & 0x1F) << 3;
    result.nV2 = ((nColor >> 10) & 0x1F) << 3;
    result.nV3 = ((nColor >> 15) & 0x01) << 7;

    return true;
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

bool CVPU::IsRunning() const
{
    return (m_vif.GetStat() & GetVbs()) != 0;
}

uint32 CVPU::GetMaskOp(unsigned int row, unsigned int col) const
{
    if(col > 3) col = 3;
    assert(row < 4);
    unsigned int index = (col * 4) + row;
    return (m_MASK >> (index * 2)) & 0x03;
}

void CVPU::DisassembleCommand(CODE code)
{
    if(m_STAT.nVPS != 0) return;

    CLog::GetInstance().Print(LOG_NAME, "vpu%i : ", m_vpuNumber);

    if(code.nI)
    {
        CLog::GetInstance().Print(LOG_NAME, "(I) ");
    }

	if(code.nCMD >= 0x60)
	{
        CLog::GetInstance().Print(LOG_NAME, "UNPACK(imm = 0x%x, num = 0x%x);\r\n", code.nIMM, code.nNUM);
	}
    else
    {
        switch(code.nCMD)
        {
        case 0x00:
            CLog::GetInstance().Print(LOG_NAME, "NOP\r\n");
            break;
        case 0x01:
            CLog::GetInstance().Print(LOG_NAME, "STCYCL(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x02:
            CLog::GetInstance().Print(LOG_NAME, "OFFSET(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x03:
            CLog::GetInstance().Print(LOG_NAME, "BASE(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x04:
            CLog::GetInstance().Print(LOG_NAME, "ITOP(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x05:
            CLog::GetInstance().Print(LOG_NAME, "STMODE(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x06:
            CLog::GetInstance().Print(LOG_NAME, "MSKPATH3();\r\n");
            break;
        case 0x11:
            CLog::GetInstance().Print(LOG_NAME, "FLUSH();\r\n");
            break;
        case 0x13:
            CLog::GetInstance().Print(LOG_NAME, "FLUSHA();\r\n");
            break;
        case 0x14:
            CLog::GetInstance().Print(LOG_NAME, "MSCAL(imm = 0x%x);\r\n", code.nIMM);
            break;
        case 0x17:
            CLog::GetInstance().Print(LOG_NAME, "MSCNT();\r\n");
            break;
        case 0x20:
            CLog::GetInstance().Print(LOG_NAME, "STMASK();\r\n");
            break;
        case 0x30:
            CLog::GetInstance().Print(LOG_NAME, "STROW();\r\n");
            break;
        case 0x4A:
            CLog::GetInstance().Print(LOG_NAME, "MPG(imm = 0x%x, num = 0x%x);\r\n", code.nIMM, code.nNUM);
            break;
        case 0x50:
            CLog::GetInstance().Print(LOG_NAME, "DIRECT(imm = 0x%x, num = 0x%x);\r\n", code.nIMM, code.nNUM);
            break;
        case 0x51:
            CLog::GetInstance().Print(LOG_NAME, "DIRECTHL(imm = 0x%x, num = 0x%x);\r\n", code.nIMM, code.nNUM);
            break;
        default:
            CLog::GetInstance().Print(LOG_NAME, "Unknown command (0x%x).\r\n", code.nCMD);
            break;
        }
    }
}
