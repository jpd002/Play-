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
m_ram(ram)
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

void CVIF::Reset()
{
    m_pVPU[0]->Reset();
    m_pVPU[1]->Reset();
    m_VPU_STAT = 0;
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

void CVIF::SingleStepVU1()
{
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
