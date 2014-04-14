#include <assert.h>
#include "VIF.h"
#include "VPU.h"
#include "VPU1.h"
#include "GIF.h"
#include "Profiler.h"
#include "Ps2Const.h"
#include "RegisterStateFile.h"
#include "Log.h"
#include "FrameDump.h"

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

#define VIF0_STAT		0x10003800
#define VIF0_FBRST		0x10003810
#define VIF0_ERR		0x10003820
#define VIF0_MARK		0x10003830
#define VIF0_CYCLE		0x10003840
#define VIF0_MODE		0x10003850
#define VIF0_NUM		0x10003860
#define VIF0_MASK		0x10003870
#define VIF0_CODE		0x10003880
#define VIF0_ITOPS		0x10003890
#define VIF0_ITOP		0x100038d0
#define VIF0_TOP		0x100038e0
#define VIF0_R0			0x10003900
#define VIF0_R1			0x10003910
#define VIF0_R2			0x10003920
#define VIF0_R3			0x10003930
#define VIF0_C0			0x10003940
#define VIF0_C1			0x10003950
#define VIF0_C2			0x10003960
#define VIF0_C3			0x10003970

#define VIF1_STAT		0x10003c00
#define VIF1_FBRST		0x10003c10
#define VIF1_ERR		0x10003c20
#define VIF1_MARK		0x10003c30
#define VIF1_CYCLE		0x10003c40
#define VIF1_MODE		0x10003c50
#define VIF1_NUM		0x10003c60
#define VIF1_MASK		0x10003c70
#define VIF1_CODE		0x10003c80
#define VIF1_ITOPS		0x10003c90
#define VIF1_BASE		0x10003ca0
#define VIF1_OFST		0x10003cb0
#define VIF1_TOPS		0x10003cc0
#define VIF1_ITOP		0x10003cd0
#define VIF1_TOP		0x10003ce0
#define VIF1_R0			0x10003d00
#define VIF1_R1			0x10003d10
#define VIF1_R2			0x10003d20
#define VIF1_R3			0x10003d30
#define VIF1_C0			0x10003d40
#define VIF1_C1			0x10003d50
#define VIF1_C2			0x10003d60
#define VIF1_C3			0x10003d70

void CVIF::SetRegister(uint32 nAddress, uint32 nValue)
{
	switch (nAddress)
	{
	case VIF1_FBRST:
		if (nValue & 1){
			m_stream[1]->Flush();
			m_pVPU[1]->Reset();
		}
		break;
	case VIF1_ERR:
		break;
	case VIF1_MARK:
		m_pVPU[1]->SetMark(nValue);
		break;

	}
}

uint32 CVIF::GetRegister(uint32 value)
{
	return 0;
}

uint32 CVIF::GetFIFO1()
{
	return 0;
}

void CVIF::SetFIFO1(uint32 value)
{
	m_stream[1]->Write(value);
	m_pVPU[1]->ProcessPacket(*m_stream[1]);
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

//	assert(nAddress < PS2::VUMEM1SIZE);

	CGsPacketMetadata metadata;
#ifdef DEBUGGER_INCLUDED
	metadata.pathIndex				= 1;
	metadata.vuMemPacketAddress		= nAddress;
	metadata.vpu1Top				= m_pVPU[1]->GetVuTopMiniState();
	metadata.vpu1Itop				= m_pVPU[1]->GetVuItopMiniState();
	memcpy(&metadata.vu1State, &m_pVPU[1]->GetVuMiniState(), sizeof(MIPSSTATE));
	memcpy(metadata.vuMem1, m_pVPU[1]->GetVuMemoryMiniState(), PS2::VUMEM1SIZE);
	memcpy(metadata.microMem1, m_pVPU[1]->GetMicroMemoryMiniState(), PS2::MICROMEM1SIZE);
#endif

	m_gif.ProcessPacket(m_pVPU[1]->GetVuMemory(), nAddress, PS2::VUMEM1SIZE, metadata);

#ifdef DEBUGGER_INCLUDED
	m_pVPU[1]->SaveMiniState();
#endif
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

void CVIF::CFifoStream::Write(uint32 data)
{
	// TODO:
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
