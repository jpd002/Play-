#include <assert.h>
#include "VIF.h"
#include "VPU.h"
#include "VPU1.h"
#include "GIF.h"
#include "Profiler.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"

#ifdef	PROFILE
#define	PROFILE_VIFZONE "VIF"
#define PROFILE_VU1ZONE	"VU0"
#endif

#define STATE_REGS_XML          ("vif/regs.xml")
#define STATE_REGS_VPU_STAT     ("VPU_STAT")

using namespace Framework;
using namespace std;
using namespace boost;

//REMOVE
static int nExecTimes = 0;

CVIF::CVIF(CGIF& gif, uint8* ram, const VPUINIT& vpu0Init, const VPUINIT& vpu1Init) :
m_gif(gif),
m_ram(ram),
m_stream(m_ram)
{
    m_pVPU[0] = new CVPU(*this, 0, vpu0Init);
    m_pVPU[1] = new CVPU1(*this, 1, vpu1Init);
}

CVIF::~CVIF()
{
    for(unsigned int i = 0; i < 2; i++)
    {
        delete m_pVPU[i];
    }
}

void CVIF::JoinThreads()
{
    m_pVPU[0]->JoinThread();
    m_pVPU[1]->JoinThread();
}

void CVIF::Reset()
{
    m_pVPU[0]->Reset();
    m_pVPU[1]->Reset();
    m_VPU_STAT = 0;
    m_stream.Flush();
}

void CVIF::SaveState(CZipArchiveWriter& archive)
{
    {
        CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
        registerFile->SetRegister32(STATE_REGS_VPU_STAT, m_VPU_STAT);
        archive.InsertFile(registerFile);
    }

    m_pVPU[0]->SaveState(archive);
    m_pVPU[1]->SaveState(archive);
}

void CVIF::LoadState(CZipArchiveReader& archive)
{
    CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
    m_VPU_STAT = registerFile.GetRegister32(STATE_REGS_VPU_STAT);

    m_pVPU[0]->LoadState(archive);
    m_pVPU[1]->LoadState(archive);
}

uint32 CVIF::ReceiveDMA1(uint32 nAddress, uint32 nQWC, bool nTagIncluded)
{
    if(IsVU1Running())
    {
        thread::yield();
        return 0;
    }

    m_stream.SetDmaParams(nAddress, nQWC * 0x10);
    if(nTagIncluded)
    {
        m_stream.Read(NULL, 8);
    }

    m_pVPU[1]->ProcessPacket(m_stream);

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_VIFZONE);
#endif

//	if(nTagIncluded)
//	{
//		nQWC = m_pVPU[1]->ProcessPacket(nAddress + 8, (nQWC * 0x10) - 8);
//	}
//	else
//	{
//		nQWC = m_pVPU[1]->ProcessPacket(nAddress, nQWC * 0x10);
//	}
	
#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

    uint32 remainingSize = m_stream.GetSize();
    assert((remainingSize & 0x0F) == 0);
    remainingSize /= 0x10;
	return nQWC - remainingSize;
}

uint32 CVIF::GetTop1()
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

bool CVIF::IsVuDebuggingEnabled() const
{
    return false;
}

bool CVIF::IsVU1Running()
{
	return (m_VPU_STAT & STAT_VBS1) != 0;
}

void CVIF::SingleStepVU1()
{
    if(!IsVuDebuggingEnabled())
    {
        return;
    }

    if(!IsVU1Running())
    {
        throw exception();
    }
    m_pVPU[1]->SingleStep();
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


CVIF::CFifoStream::CFifoStream(uint8* ram) :
m_ram(ram),
m_address(0),
m_endAddress(0),
m_nextAddress(0),
m_position(BUFFERSIZE)
{

}

CVIF::CFifoStream::~CFifoStream()
{

}

void CVIF::CFifoStream::Read(void* buffer, uint32 size)
{
    uint8* readBuffer = reinterpret_cast<uint8*>(buffer);
    while(size != 0)
    {
        SyncBuffer();
        uint32 read = min(size, BUFFERSIZE - m_position);
        if(readBuffer != NULL)
        {
            memcpy(readBuffer, reinterpret_cast<uint8*>(&m_buffer) + m_position, read);
            readBuffer += read;
        }
        m_position += read;
        size -= read;
    }
}

void CVIF::CFifoStream::Flush()
{
    m_position = BUFFERSIZE;
}

void CVIF::CFifoStream::SetDmaParams(uint32 address, uint32 qwc)
{
    m_address = address;
    m_nextAddress = address;
    m_endAddress = address + qwc;
    SyncBuffer();
}

uint32 CVIF::CFifoStream::GetAddress() const
{
    return m_address + m_position;
}

uint32 CVIF::CFifoStream::GetSize() const
{
    return m_endAddress - GetAddress();
}

void CVIF::CFifoStream::Align32()
{
    Read(NULL, 4 - (GetAddress() & 0x03));
    assert((GetAddress() & 0x03) == 0);
}

void CVIF::CFifoStream::SyncBuffer()
{
    if(m_position >= BUFFERSIZE)
    {
        if(m_nextAddress >= m_endAddress)
        {
            throw exception();
        }
        m_address = m_nextAddress;
        m_buffer = *reinterpret_cast<uint128*>(&m_ram[m_nextAddress]);
        m_nextAddress += 0x10;
        m_position = 0;
    }
}
