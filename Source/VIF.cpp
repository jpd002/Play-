#include <assert.h>
#include "VIF.h"
#include "VPU.h"
#include "VPU1.h"
#include "GIF.h"
#include "Profiler.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"
#include "Log.h"

#ifdef	PROFILE
#define	PROFILE_VIFZONE "VIF"
#define PROFILE_VU1ZONE	"VU0"
#endif

#define LOG_NAME				("vif")
#define STATE_REGS_XML			("vif/regs.xml")
#define STATE_REGS_VPU_STAT		("VPU_STAT")

CVIF::CVIF(CGIF& gif, uint8* ram, uint8* spr, const VPUINIT& vpu0Init, const VPUINIT& vpu1Init)
: m_gif(gif)
, m_ram(ram)
, m_spr(spr)
, m_VPU_STAT(0)
{
	m_pVPU[0] = new CVPU(*this, 0, vpu0Init);
	m_pVPU[1] = new CVPU1(*this, 1, vpu1Init);
	for(unsigned int i = 0; i < 2; i++)
	{
		m_stream[i] = new CFifoStream(ram, spr);
	}
	Reset();
}

CVIF::~CVIF()
{
	for(unsigned int i = 0; i < 2; i++)
	{
		delete m_pVPU[i];
		delete m_stream[i];
	}
}

void CVIF::Reset()
{
	for(unsigned int i = 0; i < 2; i++)
	{
		m_stream[i]->Flush();
		m_pVPU[i]->Reset();
	}
	m_VPU_STAT = 0;
}

void CVIF::SaveState(Framework::CZipArchiveWriter& archive)
{
	//TODO: Save FifoStream states

	{
		CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
		registerFile->SetRegister32(STATE_REGS_VPU_STAT, m_VPU_STAT);
		archive.InsertFile(registerFile);
	}

	m_pVPU[0]->SaveState(archive);
	m_pVPU[1]->SaveState(archive);
}

void CVIF::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_VPU_STAT = registerFile.GetRegister32(STATE_REGS_VPU_STAT);

	m_pVPU[0]->LoadState(archive);
	m_pVPU[1]->LoadState(archive);
}

uint32 CVIF::ReceiveDMA0(uint32 address, uint32 qwc, bool tagIncluded)
{
	unsigned int vpuNumber = 0;

	if(IsVu0Running() && IsVu0WaitingForProgramEnd())
	{
		return 0;
	}

	return ProcessDMAPacket(0, address, qwc, tagIncluded);
}

uint32 CVIF::ReceiveDMA1(uint32 address, uint32 qwc, bool tagIncluded)
{
	unsigned int vpuNumber = 1;

	if(IsVu1Running() && IsVu1WaitingForProgramEnd())
	{
		return 0;
	}

	return ProcessDMAPacket(1, address, qwc, tagIncluded);
}

uint32 CVIF::ProcessDMAPacket(unsigned int vpuNumber, uint32 address, uint32 qwc, bool tagIncluded)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "vif%i : Processing packet @ 0x%0.8X, qwc = 0x%X, tagIncluded = %i\r\n",
		vpuNumber, address, qwc, static_cast<int>(tagIncluded));
#endif

#ifdef PROFILE
	CProfilerZone profilerZone(PROFILE_VIFZONE);
#endif

	m_stream[vpuNumber]->SetDmaParams(address, qwc * 0x10);
	if(tagIncluded)
	{
		m_stream[vpuNumber]->Read(NULL, 8);
	}

	m_pVPU[vpuNumber]->ProcessPacket(*m_stream[vpuNumber]);

	uint32 remainingSize = m_stream[vpuNumber]->GetRemainingDmaTransferSize();
	assert((remainingSize & 0x0F) == 0);
	remainingSize /= 0x10;

	return qwc - remainingSize;
}

uint32 CVIF::GetITop0() const
{
	return m_pVPU[0]->GetITOP();
}

uint32 CVIF::GetITop1() const
{
	return m_pVPU[1]->GetITOP();
}

uint32 CVIF::GetTop1() const
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
	nAddress &= 0x3FF;
	nAddress *= 0x10;

//    assert(nAddress < PS2::VUMEM1SIZE);

	m_gif.ProcessPacket(m_pVPU[1]->GetVuMemory(), nAddress, PS2::VUMEM1SIZE);
}

void CVIF::StartVu0MicroProgram(uint32 address)
{
	m_pVPU[0]->StartMicroProgram(address);
}

bool CVIF::IsVu0Running() const
{
	return m_pVPU[0]->IsRunning();
}

bool CVIF::IsVu1Running() const
{
	return m_pVPU[1]->IsRunning();
}

bool CVIF::IsVu0WaitingForProgramEnd() const
{
	return m_pVPU[0]->IsWaitingForProgramEnd();
}

bool CVIF::IsVu1WaitingForProgramEnd() const
{
	return m_pVPU[1]->IsWaitingForProgramEnd();
}

void CVIF::ExecuteVu0(bool singleStep)
{
	if(!IsVu0Running()) return;
#ifdef PROFILE
	CProfilerZone profilerZone(PROFILE_VIFZONE);
#endif
	m_pVPU[0]->Execute(singleStep);
}

void CVIF::ExecuteVu1(bool singleStep)
{
	if(!IsVu1Running()) return;
#ifdef PROFILE
	CProfilerZone profilerZone(PROFILE_VIFZONE);
#endif	
	m_pVPU[1]->Execute(singleStep);
}

#ifdef DEBUGGER_INCLUDED

bool CVIF::MustVu1Break() const
{
	if(!IsVu1Running()) return false;
	return m_pVPU[1]->MustBreak();
}

void CVIF::DisableVu1BreakpointsOnce()
{
	m_pVPU[1]->DisableBreakpointsOnce();
}

#endif

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


CVIF::CFifoStream::CFifoStream(uint8* ram, uint8* spr) 
: m_ram(ram)
, m_spr(spr)
, m_source(NULL)
, m_endAddress(0)
, m_nextAddress(0)
, m_bufferPosition(BUFFERSIZE)
{

}

CVIF::CFifoStream::~CFifoStream()
{

}

void CVIF::CFifoStream::Read(void* buffer, uint32 size)
{
	assert(m_source != NULL);
	uint8* readBuffer = reinterpret_cast<uint8*>(buffer);
	while(size != 0)
	{
		SyncBuffer();
		uint32 read = std::min<uint32>(size, BUFFERSIZE - m_bufferPosition);
		if(readBuffer != NULL)
		{
			memcpy(readBuffer, reinterpret_cast<uint8*>(&m_buffer) + m_bufferPosition, read);
			readBuffer += read;
		}
		m_bufferPosition += read;
		size -= read;
	}
}

void CVIF::CFifoStream::Flush()
{
	m_bufferPosition = BUFFERSIZE;
}

void CVIF::CFifoStream::SetDmaParams(uint32 address, uint32 size)
{
	if(address & 0x80000000)
	{
		m_source = m_spr;
		address &= (PS2::EE_SPR_SIZE - 1);
		assert((address + size) <= PS2::EE_SPR_SIZE);
	}
	else
	{
		m_source = m_ram;
		address &= (PS2::EE_RAM_SIZE - 1);
		assert((address + size) <= PS2::EE_RAM_SIZE);
	}
	m_nextAddress = address;
	m_endAddress = address + size;
	SyncBuffer();
}

uint32 CVIF::CFifoStream::GetAvailableReadBytes() const
{
	return GetRemainingDmaTransferSize() + (BUFFERSIZE - m_bufferPosition);
}

uint32 CVIF::CFifoStream::GetRemainingDmaTransferSize() const
{
	return m_endAddress - m_nextAddress;
}

void CVIF::CFifoStream::Align32()
{
	unsigned int remainBytes = m_bufferPosition & 0x03;
	if(remainBytes == 0) return;
	Read(NULL, 4 - remainBytes);
	assert((m_bufferPosition & 0x03) == 0);
}

void CVIF::CFifoStream::SyncBuffer()
{
	assert(m_bufferPosition <= BUFFERSIZE);
	if(m_bufferPosition >= BUFFERSIZE)
	{
		if(m_nextAddress >= m_endAddress)
		{
			throw std::exception();
		}
		m_buffer = *reinterpret_cast<uint128*>(&m_source[m_nextAddress]);
		m_nextAddress += 0x10;
		m_bufferPosition = 0;
	}
}
