#include "make_unique.h"
#include "../Log.h"
#include "../RegisterStateFile.h"
#include "../Ps2Const.h"
#include "../FrameDump.h"
#include "Vif.h"
#include "Vif1.h"
#include "GIF.h"
#include "Vpu.h"

#define LOG_NAME				("vpu")

CVpu::CVpu(unsigned int number, const VPUINIT& vpuInit, CGIF& gif, uint8* ram, uint8* spr)
: m_number(number)
, m_vif((number == 0) ? std::make_unique<CVif>(0, *this, ram, spr) : std::make_unique<CVif1>(1, *this, gif, ram, spr))
, m_microMem(vpuInit.microMem)
, m_vuMem(vpuInit.vuMem)
, m_ctx(vpuInit.context)
, m_gif(gif)
, m_executor(*vpuInit.context)
, m_vuProfilerZone(CProfiler::GetInstance().RegisterZone("VU"))
#ifdef DEBUGGER_INCLUDED
, m_microMemMiniState(new uint8[(number == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE])
, m_vuMemMiniState(new uint8[(number == 0) ? PS2::VUMEM0SIZE : PS2::VUMEM1SIZE])
, m_topMiniState(0)
, m_itopMiniState(0)
#endif
{

}

CVpu::~CVpu()
{
#ifdef DEBUGGER_INCLUDED
	delete [] m_microMemMiniState;
	delete [] m_vuMemMiniState;
#endif
}

void CVpu::Execute(bool singleStep)
{
	if(!m_running) return;

#ifdef PROFILE
	CProfilerZone profilerZone(m_vuProfilerZone);
#endif

	unsigned int quota = singleStep ? 1 : 5000;
	m_executor.Execute(quota);
	if(m_ctx->m_State.nHasException)
	{
		//E bit encountered
		m_running = false;
	}
}

#ifdef DEBUGGER_INCLUDED

bool CVpu::MustBreak() const
{
	return m_executor.MustBreak();
}

void CVpu::DisableBreakpointsOnce()
{
	m_executor.DisableBreakpointsOnce();
}

void CVpu::SaveMiniState()
{
	memcpy(m_microMemMiniState, m_microMem, (m_number == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE);
	memcpy(m_vuMemMiniState, m_vuMem, (m_number == 0) ? PS2::VUMEM0SIZE : PS2::VUMEM1SIZE);
	memcpy(&m_vuMiniState, &m_ctx->m_State, sizeof(MIPSSTATE));
	m_topMiniState = (m_number == 0) ? 0 : m_vif->GetTOP();
	m_itopMiniState = m_vif->GetITOP();
}

const MIPSSTATE& CVpu::GetVuMiniState() const
{
	return m_vuMiniState;
}

uint8* CVpu::GetVuMemoryMiniState() const
{
	return m_vuMemMiniState;
}

uint8* CVpu::GetMicroMemoryMiniState() const
{
	return m_microMemMiniState;
}

uint32 CVpu::GetVuTopMiniState() const
{
	return m_topMiniState;
}

uint32 CVpu::GetVuItopMiniState() const
{
	return m_itopMiniState;
}

#endif

void CVpu::Reset()
{
	m_running = false;
	m_executor.Reset();
	m_vif->Reset();
}

void CVpu::SaveState(Framework::CZipArchiveWriter& archive)
{
	m_vif->SaveState(archive);
}

void CVpu::LoadState(Framework::CZipArchiveReader& archive)
{
	m_vif->LoadState(archive);
}

CMIPS& CVpu::GetContext() const
{
	return *m_ctx;
}

uint8* CVpu::GetMicroMemory() const
{
	return m_microMem;
}

uint8* CVpu::GetVuMemory() const
{
	return m_vuMem;
}

bool CVpu::IsVuRunning() const
{
	return m_running;
}

CVif& CVpu::GetVif()
{
	return *m_vif.get();
}

void CVpu::ExecuteMicroProgram(uint32 nAddress)
{
	CLog::GetInstance().Print(LOG_NAME, "Starting microprogram execution at 0x%0.8X.\r\n", nAddress);

	m_ctx->m_State.nPC = nAddress;
	m_ctx->m_State.pipeTime = 0;
	m_ctx->m_State.nHasException = 0;

#ifdef DEBUGGER_INCLUDED
	SaveMiniState();
#endif

	assert(!m_running);
	m_running = true;
	for(unsigned int i = 0; i < 100; i++)
	{
		Execute(false);
		if(!m_running) break;
	}
}

void CVpu::InvalidateMicroProgram()
{
	m_executor.ClearActiveBlocks();
}

void CVpu::ProcessXgKick(uint32 address)
{
	address &= 0x3FF;
	address *= 0x10;

//	assert(nAddress < PS2::VUMEM1SIZE);

	CGsPacketMetadata metadata;
#ifdef DEBUGGER_INCLUDED
	metadata.pathIndex				= 1;
	metadata.vuMemPacketAddress		= address;
	metadata.vpu1Top				= GetVuTopMiniState();
	metadata.vpu1Itop				= GetVuItopMiniState();
	memcpy(&metadata.vu1State, &GetVuMiniState(), sizeof(MIPSSTATE));
	memcpy(metadata.vuMem1, GetVuMemoryMiniState(), PS2::VUMEM1SIZE);
	memcpy(metadata.microMem1, GetMicroMemoryMiniState(), PS2::MICROMEM1SIZE);
#endif

	m_gif.ProcessPacket(GetVuMemory(), address, PS2::VUMEM1SIZE, metadata);

#ifdef DEBUGGER_INCLUDED
	SaveMiniState();
#endif
}
