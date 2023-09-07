#include "Vpu.h"
#include "make_unique.h"
#include "string_format.h"
#include "../Log.h"
#include "../states/RegisterStateFile.h"
#include "../Ps2Const.h"
#include "../FrameDump.h"
#include "Vif.h"
#include "Vif1.h"
#include "GIF.h"

#define LOG_NAME ("ee_vpu")

#define STATE_PATH_REGS_FORMAT ("vpu/vpu_%d.xml")

#define STATE_REGS_RUNNING ("running")

CVpu::CVpu(unsigned int number, const VPUINIT& vpuInit, CGIF& gif, CINTC& intc, uint8* ram, uint8* spr)
    : m_number(number)
    , m_vif((number == 0) ? std::make_unique<CVif>(0, *this, intc, ram, spr) : std::make_unique<CVif1>(1, *this, gif, intc, ram, spr))
    , m_microMem(vpuInit.microMem)
    , m_microMemSize((number == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE)
    , m_vuMem(vpuInit.vuMem)
    , m_vuMemSize((number == 0) ? PS2::VUMEM0SIZE : PS2::VUMEM1SIZE)
    , m_ctx(vpuInit.context)
    , m_gif(gif)
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
	delete[] m_microMemMiniState;
	delete[] m_vuMemMiniState;
#endif
}

void CVpu::Execute(int32 quota)
{
	if(!m_running) return;

#ifdef PROFILE
	CProfilerZone profilerZone(m_vuProfilerZone);
#endif

	m_ctx->m_executor->Execute(quota);
	if(m_ctx->m_State.nHasException)
	{
		//E bit encountered
		m_running = false;
		VuStateChanged(m_running);
	}
}

#ifdef DEBUGGER_INCLUDED

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
	m_ctx->m_executor->Reset();
	m_vif->Reset();
}

void CVpu::SaveState(Framework::CZipArchiveWriter& archive)
{
	{
		auto path = string_format(STATE_PATH_REGS_FORMAT, m_number);
		auto registerFile = std::make_unique<CRegisterStateFile>(path.c_str());
		registerFile->SetRegister32(STATE_REGS_RUNNING, m_running);
		archive.InsertFile(std::move(registerFile));
	}

	m_vif->SaveState(archive);
}

void CVpu::LoadState(Framework::CZipArchiveReader& archive)
{
	{
		auto path = string_format(STATE_PATH_REGS_FORMAT, m_number);
		CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
		m_running = registerFile.GetRegister32(STATE_REGS_RUNNING) != 0;
	}

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

uint32 CVpu::GetMicroMemorySize() const
{
	return m_microMemSize;
}

uint8* CVpu::GetVuMemory() const
{
	return m_vuMem;
}

uint32 CVpu::GetVuMemorySize() const
{
	return m_vuMemSize;
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
	CLog::GetInstance().Print(LOG_NAME, "Starting microprogram execution at 0x%08X.\r\n", nAddress);

	m_ctx->m_State.nPC = nAddress;
	m_ctx->m_State.pipeTime = 0;
	m_ctx->m_State.nHasException = 0;

#ifdef DEBUGGER_INCLUDED
	SaveMiniState();
#endif

	assert(!m_running);
	m_running = true;
	VuStateChanged(m_running);
	for(unsigned int i = 0; i < 100; i++)
	{
		Execute(5000);
		if(!m_running) break;
	}
}

void CVpu::InvalidateMicroProgram()
{
	m_ctx->m_executor->ClearActiveBlocksInRange(0, (m_number == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE, false);
}

void CVpu::InvalidateMicroProgram(uint32 start, uint32 end)
{
	m_ctx->m_executor->ClearActiveBlocksInRange(start, end, false);
}

void CVpu::ProcessXgKick(uint32 address)
{
	address &= 0x3FF;
	address *= 0x10;

	CGsPacketMetadata metadata;
	metadata.pathIndex = 1;
#ifdef DEBUGGER_INCLUDED
	metadata.vuMemPacketAddress = address;
	metadata.vpu1Top = GetVuTopMiniState();
	metadata.vpu1Itop = GetVuItopMiniState();
	memcpy(&metadata.vu1State, &GetVuMiniState(), sizeof(MIPSSTATE));
	memcpy(metadata.vuMem1, GetVuMemoryMiniState(), PS2::VUMEM1SIZE);
	memcpy(metadata.microMem1, GetMicroMemoryMiniState(), PS2::MICROMEM1SIZE);
#endif

	address += m_gif.ProcessSinglePacket(GetVuMemory(), PS2::VUMEM1SIZE, address, PS2::VUMEM1SIZE, metadata);
	if((address == PS2::VUMEM1SIZE) && (m_gif.GetActivePath() == 1))
	{
		address = 0;
		address += m_gif.ProcessSinglePacket(GetVuMemory(), PS2::VUMEM1SIZE, address, PS2::VUMEM1SIZE, metadata);
	}
	assert(m_gif.GetActivePath() == 0);

#ifdef DEBUGGER_INCLUDED
	SaveMiniState();
#endif
}
