#include <time.h>
#include "Ee_SubSystem.h"
#include "EeExecutor.h"
#include "VuExecutor.h"
#include "AppConfig.h"
#include "StdStreamUtils.h"
#include "../Ps2Const.h"
#include "Log.h"
#include "../states/MemoryStateFile.h"
#include "../iop/IopBios.h"
#include "Vif.h"
#include "placeholder_def.h"

using namespace Ee;

#define LOG_NAME ("ee_subsystem")

#define STATE_EE ("ee")
#define STATE_VU0 ("vu0")
#define STATE_VU1 ("vu1")
#define STATE_RAM ("ram")
#define STATE_SPR ("spr")
#define STATE_VUMEM0 ("vumem0")
#define STATE_MICROMEM0 ("micromem0")
#define STATE_VUMEM1 ("vumem1")
#define STATE_MICROMEM1 ("micromem1")

#define FAKE_IOP_RAM_SIZE (0x1000)

CSubSystem::CSubSystem(uint8* iopRam, CIopBios& iopBios)
    : m_ram(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::EE_RAM_SIZE, framework_getpagesize())))
    , m_bios(new uint8[PS2::EE_BIOS_SIZE])
    , m_spr(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::EE_SPR_SIZE, 0x10)))
    , m_fakeIopRam(new uint8[FAKE_IOP_RAM_SIZE])
    , m_vuMem0(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::VUMEM0SIZE, 0x10)))
    , m_microMem0(new uint8[PS2::MICROMEM0SIZE])
    , m_vuMem1(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::VUMEM1SIZE, 0x10)))
    , m_microMem1(new uint8[PS2::MICROMEM1SIZE])
    , m_EE(MEMORYMAP_ENDIAN_LSBF, true)
    , m_VU0(MEMORYMAP_ENDIAN_LSBF)
    , m_VU1(MEMORYMAP_ENDIAN_LSBF)
    , m_dmac(m_ram, m_spr, m_vuMem0, m_vuMem1, m_EE)
    , m_gif(m_gs, m_dmac, m_ram, m_spr)
    , m_sif(m_dmac, m_ram, iopRam)
    , m_ipu(m_intc)
    , m_timer(m_intc, m_gs)
    , m_iopBios(iopBios)
    , m_MAVU0(PS2::VUMEM0SIZE - 1)
    , m_MAVU1(PS2::VUMEM1SIZE - 1)
    , m_COP_SCU(MIPS_REGSIZE_64)
    , m_COP_FPU(MIPS_REGSIZE_64)
    , m_COP_VU(MIPS_REGSIZE_64)
{
	//Some alignment checks, this is needed because of SIMD instructions used in generated code
	assert((reinterpret_cast<size_t>(&m_EE.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(&m_VU0.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(&m_VU1.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(m_spr) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(m_vuMem0) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(m_vuMem1) & 0x0F) == 0);

	m_vpu0 = std::make_shared<CVpu>(0, CVpu::VPUINIT(m_microMem0, m_vuMem0, &m_VU0), m_gif, m_intc, m_ram, m_spr);
	m_vpu1 = std::make_shared<CVpu>(1, CVpu::VPUINIT(m_microMem1, m_vuMem1, &m_VU1), m_gif, m_intc, m_ram, m_spr);

	//Setup link between EE's VU context and VU0's VU context
	m_vu0StateChangedConnection = m_vpu0->VuStateChanged.Connect([this](CVpu::VU_STATE newState) { Vu0StateChanged(newState); });

	m_vu1InterruptTriggeredConnection = m_vpu1->VuInterruptTriggered.Connect(
	    [this]() {
		    uint32 currentState = m_intc.GetRegister(CINTC::INTC_STAT);
		    assert((currentState & (1 << CINTC::INTC_LINE_VU1)) == 0);
		    m_intc.AssertLine(CINTC::INTC_LINE_VU1);
	    });

	//EmotionEngine context setup
	{
		m_EE.m_executor = std::make_unique<CEeExecutor>(m_EE, m_ram);

		//Read map
		m_EE.m_pMemoryMap->InsertReadMap(0x00000000, PS2::EE_RAM_SIZE - 1, m_ram, 0x00);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::EE_SPR_ADDR, PS2::EE_SPR_ADDR + PS2::EE_SPR_SIZE - 1, m_spr, 0x01);
		m_EE.m_pMemoryMap->InsertReadMap(0x10000000, 0x10FFFFFF, std::bind(&CSubSystem::IOPortReadHandler, this, PLACEHOLDER_1), 0x02);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::MICROMEM0ADDR, PS2::MICROMEM0ADDR + PS2::MICROMEM0SIZE - 1, m_microMem0, 0x03);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::VUMEM0ADDR, PS2::VUMEM0ADDR + PS2::VUMEM0SIZE - 1, m_vuMem0, 0x04);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::MICROMEM1ADDR, PS2::MICROMEM1ADDR + PS2::MICROMEM1SIZE - 1, m_microMem1, 0x05);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::VUMEM1ADDR, PS2::VUMEM1ADDR + PS2::VUMEM1SIZE - 1, m_vuMem1, 0x06);
		m_EE.m_pMemoryMap->InsertReadMap(0x12000000, 0x12FFFFFF, std::bind(&CSubSystem::IOPortReadHandler, this, PLACEHOLDER_1), 0x07);
		m_EE.m_pMemoryMap->InsertReadMap(0x1C000000, 0x1C001000, m_fakeIopRam, 0x08);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::EE_BIOS_ADDR, PS2::EE_BIOS_ADDR + PS2::EE_BIOS_SIZE - 1, m_bios, 0x09);

		//Write map
		m_EE.m_pMemoryMap->InsertWriteMap(0x00000000, PS2::EE_RAM_SIZE - 1, m_ram, 0x00);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::EE_SPR_ADDR, PS2::EE_SPR_ADDR + PS2::EE_SPR_SIZE - 1, m_spr, 0x01);
		m_EE.m_pMemoryMap->InsertWriteMap(0x10000000, 0x10FFFFFF, std::bind(&CSubSystem::IOPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x02);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::MICROMEM0ADDR, PS2::MICROMEM0ADDR + PS2::MICROMEM0SIZE - 1, std::bind(&CSubSystem::Vu0MicroMemWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x03);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::VUMEM0ADDR, PS2::VUMEM0ADDR + PS2::VUMEM0SIZE - 1, m_vuMem0, 0x04);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::MICROMEM1ADDR, PS2::MICROMEM1ADDR + PS2::MICROMEM1SIZE - 1, std::bind(&CSubSystem::Vu1MicroMemWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x05);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::VUMEM1ADDR, PS2::VUMEM1ADDR + PS2::VUMEM1SIZE - 1, m_vuMem1, 0x06);
		m_EE.m_pMemoryMap->InsertWriteMap(0x12000000, 0x12FFFFFF, std::bind(&CSubSystem::IOPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x07);

		//Instruction map
		m_EE.m_pMemoryMap->InsertInstructionMap(0x00000000, PS2::EE_RAM_SIZE - 1, m_ram, 0x00);
		m_EE.m_pMemoryMap->InsertInstructionMap(PS2::EE_BIOS_ADDR, PS2::EE_BIOS_ADDR + PS2::EE_BIOS_SIZE - 1, m_bios, 0x01);

		m_EE.m_pArch = &m_EEArch;
		m_EE.m_pCOP[0] = &m_COP_SCU;
		m_EE.m_pCOP[1] = &m_COP_FPU;
		m_EE.m_pCOP[2] = &m_COP_VU;

		m_EE.m_pAddrTranslator = CPS2OS::TranslateAddress;
	}

	//Vector Unit 0 context setup
	{
		m_VU0.m_executor = std::make_unique<CVuExecutor>(m_VU0, PS2::MICROMEM0SIZE);

		m_VU0.m_pMemoryMap->InsertReadMap(0x00000000, 0x00000FFF, m_vuMem0, 0x01);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00001000, 0x00001FFF, m_vuMem0, 0x02);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00002000, 0x00002FFF, m_vuMem0, 0x03);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00003000, 0x00003FFF, m_vuMem0, 0x04);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00004000, 0x00008FFF, std::bind(&CSubSystem::Vu0IoPortReadHandler, this, PLACEHOLDER_1), 0x05);

		m_VU0.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00000FFF, m_vuMem0, 0x01);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00001000, 0x00001FFF, m_vuMem0, 0x02);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00002000, 0x00002FFF, m_vuMem0, 0x03);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00003000, 0x00003FFF, m_vuMem0, 0x04);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00004000, 0x00008FFF, std::bind(&CSubSystem::Vu0IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x05);

		m_VU0.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00000FFF, m_microMem0, 0x00);

		m_VU0.m_pArch = &m_MAVU0;
		m_VU0.m_pAddrTranslator = CMIPS::TranslateAddress64;
	}

	//Vector Unit 1 context setup
	{
		m_VU1.m_executor = std::make_unique<CVuExecutor>(m_VU1, PS2::MICROMEM1SIZE);

		m_VU1.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_vuMem1, 0x00);
		m_VU1.m_pMemoryMap->InsertReadMap(0x00008000, 0x00008FFF, std::bind(&CSubSystem::Vu1IoPortReadHandler, this, PLACEHOLDER_1), 0x01);

		m_VU1.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_vuMem1, 0x00);
		m_VU1.m_pMemoryMap->InsertWriteMap(0x00008000, 0x00008FFF, std::bind(&CSubSystem::Vu1IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2), 0x01);

		m_VU1.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00003FFF, m_microMem1, 0x01);

		m_VU1.m_pArch = &m_MAVU1;
		m_VU1.m_pAddrTranslator = CMIPS::TranslateAddress64;
	}

	m_EE.m_vuMem = m_vuMem0;
	m_VU0.m_vuMem = m_vuMem0;
	m_VU1.m_vuMem = m_vuMem1;

	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_VIF0, std::bind(&CVif::ReceiveDMA, &m_vpu0->GetVif(), PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_VIF1, std::bind(&CVif::ReceiveDMA, &m_vpu1->GetVif(), PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_GIF, std::bind(&CGIF::ReceiveDMA, &m_gif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_TO_IPU, std::bind(&CIPU::ReceiveDMA4, &m_ipu, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4, m_ram, m_spr));
	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_SIF0, std::bind(&CSIF::ReceiveDMA5, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(CDMAC::CHANNEL_ID_SIF1, std::bind(&CSIF::ReceiveDMA6, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));

	m_ipu.SetDMA3ReceiveHandler(std::bind(&CDMAC::ResumeDMA3, &m_dmac, PLACEHOLDER_1, PLACEHOLDER_2));

	m_os = new CPS2OS(m_EE, m_ram, m_bios, m_spr, m_gs, m_sif, iopBios);
	m_OnRequestInstructionCacheFlushConnection = m_os->OnRequestInstructionCacheFlush.Connect(std::bind(&CSubSystem::FlushInstructionCache, this));

	SetupEePageTable();
}

CSubSystem::~CSubSystem()
{
	m_EE.m_executor->Reset();
	delete m_os;
	framework_aligned_free(m_ram);
	delete[] m_bios;
	framework_aligned_free(m_spr);
	delete[] m_fakeIopRam;
	framework_aligned_free(m_vuMem0);
	delete[] m_microMem0;
	framework_aligned_free(m_vuMem1);
	delete[] m_microMem1;
}

void CSubSystem::SetVpu0(std::shared_ptr<CVpu> newVpu0)
{
	m_vpu0 = newVpu0;
}

void CSubSystem::SetVpu1(std::shared_ptr<CVpu> newVpu1)
{
	m_vpu1 = newVpu1;
}

void CSubSystem::Reset(uint32 ramSize)
{
	m_os->Release();
	m_EE.m_executor->Reset();

	memset(m_ram, 0, PS2::EE_RAM_SIZE);
	memset(m_spr, 0, PS2::EE_SPR_SIZE);
	memset(m_bios, 0, PS2::EE_BIOS_SIZE);
	memset(m_fakeIopRam, 0, FAKE_IOP_RAM_SIZE);
	memset(m_vuMem0, 0, PS2::VUMEM0SIZE);
	memset(m_microMem0, 0, PS2::MICROMEM0SIZE);
	memset(m_vuMem1, 0, PS2::VUMEM1SIZE);
	memset(m_microMem1, 0, PS2::MICROMEM1SIZE);

	//Reset Contexts
	m_EE.Reset();
	m_VU0.Reset();
	m_VU1.Reset();

	m_EE.m_Comments.RemoveTags();
	m_EE.m_Functions.RemoveTags();
	m_VU0.m_Comments.RemoveTags();
	m_VU0.m_Functions.RemoveTags();
	m_VU1.m_Comments.RemoveTags();
	m_VU1.m_Functions.RemoveTags();

	//Reset subunits
	m_sif.Reset();
	m_ipu.Reset();
	m_gif.Reset();
	m_vpu0->Reset();
	m_vpu1->Reset();
	m_dmac.Reset();
	m_intc.Reset();
	m_timer.Reset();

	m_os->Initialize(ramSize);
	m_os->GetLibMc2().Reset();
	FillFakeIopRam();
	//LoadBIOS();

	m_statusRegisterCheckers.clear();
	m_isIdle = false;
}

int CSubSystem::ExecuteCpu(int quota)
{
	m_isIdle = false;
	int executed = 0;
	if(m_EE.m_State.callMsEnabled)
	{
		if(m_vpu0->IsVuReady())
		{
			//callMs mode over
			m_EE.m_State.callMsAddr = m_VU0.m_State.nPC;
			m_EE.m_State.callMsEnabled = 0;
		}
	}
	else if(!m_EE.m_State.nHasException)
	{
		executed = (quota - m_EE.m_executor->Execute(quota));
	}
	if(m_EE.m_State.nHasException)
	{
		switch(m_EE.m_State.nHasException)
		{
		case MIPS_EXCEPTION_SYSCALL:
			m_os->HandleSyscall();
			break;
		case MIPS_EXCEPTION_TLB:
			m_os->HandleTLBException();
			break;
		case MIPS_EXCEPTION_VU_CALLMS:
			assert(m_EE.m_State.callMsEnabled);
			if(m_EE.m_State.callMsEnabled)
			{
				//We are in callMs mode
				m_vpu0->ExecuteMicroProgram(m_EE.m_State.callMsAddr);
				m_EE.m_State.nHasException = MIPS_EXCEPTION_NONE;
			}
			break;
		case MIPS_EXCEPTION_IDLE:
		{
			m_isIdle = true;
			m_EE.m_State.nHasException = MIPS_EXCEPTION_NONE;
		}
		break;
		case MIPS_EXCEPTION_CHECKPENDINGINT:
		{
			m_EE.m_State.nHasException = MIPS_EXCEPTION_NONE;
			CheckPendingInterrupts();
		}
		break;
		case MIPS_EXCEPTION_RETURNFROMEXCEPTION:
		{
			m_EE.m_State.nHasException = MIPS_EXCEPTION_NONE;
			m_os->HandleReturnFromException();
			CheckPendingInterrupts();
		}
		break;
		default:
			assert(0);
			break;
		}
		assert(!m_EE.m_State.nHasException);
	}
	return executed;
}

bool CSubSystem::IsCpuIdle() const
{
	return m_os->IsIdle() || m_isIdle;
}

void CSubSystem::CountTicks(int ticks)
{
	if(m_vpu0->IsVuReady() || (m_vpu0->IsVuRunning() && !m_vpu0->GetVif().IsWaitingForProgramEnd()))
	{
		m_dmac.ResumeDMA0();
	}
	if(m_vpu1->IsVuReady() || (m_vpu1->IsVuRunning() && !m_vpu1->GetVif().IsWaitingForProgramEnd()))
	{
		m_dmac.ResumeDMA1();
	}
	m_dmac.ResumeDMA2();
	m_dmac.ResumeDMA8();
	m_gif.CountTicks(ticks);
	m_ipu.CountTicks(ticks);
	m_vpu0->GetVif().CountTicks(ticks);
	m_vpu1->GetVif().CountTicks(ticks);
	ExecuteIpu();
	if(!m_EE.m_State.nHasException)
	{
		if((m_EE.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0)
		{
			m_sif.CountTicks(ticks);
		}
	}
	m_EE.m_State.nCOP0[CCOP_SCU::COUNT] += ticks;
	m_timer.Count(ticks);
	if(m_EE.m_State.cop0_pccr & 0x80000000)
	{
		auto pccr = make_convertible<CCOP_SCU::PCCR>(m_EE.m_State.cop0_pccr);
		bool event0Enabled = (pccr.u0 | pccr.s0 | pccr.k0 | pccr.exl0) != 0;
		bool event1Enabled = (pccr.u1 | pccr.s1 | pccr.k1 | pccr.exl1) != 0;
		if(event0Enabled && (pccr.event0 == 1))
		{
			m_EE.m_State.cop0_pcr[0] += ticks;
		}
		if(event1Enabled && (pccr.event1 == 1))
		{
			m_EE.m_State.cop0_pcr[1] += ticks;
		}
	}
	CheckPendingInterrupts();
}

void CSubSystem::NotifyVBlankStart()
{
	m_timer.NotifyVBlankStart();
	m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_START);
	m_os->GetLibMc2().NotifyVBlankStart();
	if(m_os->CheckVBlankFlag())
	{
		//Make sure a vblank start interrupt is serviced now because
		//if vsync flag was set, we want to make sure interrupt is caught
		CheckPendingInterrupts();
	}
}

void CSubSystem::NotifyVBlankEnd()
{
	m_timer.NotifyVBlankEnd();
	m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_END);
}

void CSubSystem::SaveState(Framework::CZipArchiveWriter& archive)
{
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_EE, &m_EE.m_State, sizeof(MIPSSTATE)));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_VU0, &m_VU0.m_State, sizeof(MIPSSTATE)));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_VU1, &m_VU1.m_State, sizeof(MIPSSTATE)));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_RAM, m_ram, PS2::EE_RAM_SIZE));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_SPR, m_spr, PS2::EE_SPR_SIZE));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_VUMEM0, m_vuMem0, PS2::VUMEM0SIZE));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_MICROMEM0, m_microMem0, PS2::MICROMEM0SIZE));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_VUMEM1, m_vuMem1, PS2::VUMEM1SIZE));
	archive.InsertFile(std::make_unique<CMemoryStateFile>(STATE_MICROMEM1, m_microMem1, PS2::MICROMEM1SIZE));

	m_dmac.SaveState(archive);
	m_intc.SaveState(archive);
	m_sif.SaveState(archive);
	m_vpu0->SaveState(archive);
	m_vpu1->SaveState(archive);
	m_timer.SaveState(archive);
	m_gif.SaveState(archive);
	m_ipu.SaveState(archive);
	m_os->GetLibMc2().SaveState(archive);
}

void CSubSystem::LoadState(Framework::CZipArchiveReader& archive)
{
	m_EE.m_executor->ClearActiveBlocksInRange(0, PS2::EE_RAM_SIZE, false);
	m_vpu0->GetContext().m_executor->ClearActiveBlocksInRange(0, PS2::MICROMEM0SIZE, false);
	m_vpu1->GetContext().m_executor->ClearActiveBlocksInRange(0, PS2::MICROMEM1SIZE, false);

	archive.BeginReadFile(STATE_EE)->Read(&m_EE.m_State, sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_VU0)->Read(&m_VU0.m_State, sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_VU1)->Read(&m_VU1.m_State, sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_RAM)->Read(m_ram, PS2::EE_RAM_SIZE);
	archive.BeginReadFile(STATE_SPR)->Read(m_spr, PS2::EE_SPR_SIZE);
	archive.BeginReadFile(STATE_VUMEM0)->Read(m_vuMem0, PS2::VUMEM0SIZE);
	archive.BeginReadFile(STATE_MICROMEM0)->Read(m_microMem0, PS2::MICROMEM0SIZE);
	archive.BeginReadFile(STATE_VUMEM1)->Read(m_vuMem1, PS2::VUMEM1SIZE);
	archive.BeginReadFile(STATE_MICROMEM1)->Read(m_microMem1, PS2::MICROMEM1SIZE);

	m_dmac.LoadState(archive);
	m_intc.LoadState(archive);
	m_sif.LoadState(archive);
	m_vpu0->LoadState(archive);
	m_vpu1->LoadState(archive);
	m_timer.LoadState(archive);
	m_gif.LoadState(archive);
	m_ipu.LoadState(archive);
	m_os->GetLibMc2().LoadState(archive);
}

void CSubSystem::SetupEePageTable()
{
	m_EE.MapPages(0x00000000, PS2::EE_RAM_SIZE, m_ram);
	m_EE.MapPages(0x20000000, PS2::EE_RAM_SIZE, m_ram); //Uncached
	m_EE.MapPages(0x30000000, PS2::EE_RAM_SIZE, m_ram); //Uncached + Accelerated
	m_EE.MapPages(0x70000000, PS2::EE_SPR_SIZE, m_spr);
	m_EE.MapPages(0x80000000, PS2::EE_RAM_SIZE, m_ram);
}

uint32 CSubSystem::IOPortReadHandler(uint32 nAddress)
{
	uint32 nReturn = 0;
	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		nReturn = m_timer.GetRegister(nAddress);
	}
	else if(nAddress >= 0x10001900 && nAddress < 0x10001904)
	{
		nReturn = (uint32)clock();
	}
	else if(nAddress >= 0x10001904 && nAddress < 0x10001908)
	{
		nReturn = CLOCKS_PER_SEC;
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
		nReturn = m_ipu.GetRegister(nAddress);
	}
	else if(nAddress >= CGIF::REGS_START && nAddress < CGIF::REGS_END)
	{
		nReturn = m_gif.GetRegister(nAddress);
	}
	else if(nAddress >= CVif::REGS0_START && nAddress < CVif::REGS0_END)
	{
		nReturn = m_vpu0->GetVif().GetRegister(nAddress);
	}
	else if(nAddress >= CVif::REGS1_START && nAddress < CVif::REGS1_END)
	{
		nReturn = m_vpu1->GetVif().GetRegister(nAddress);
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		nReturn = m_intc.GetRegister(nAddress);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		nReturn = m_dmac.GetRegister(nAddress);
	}
	else if(nAddress >= CVpu::EE_ADDR_VU1AREA_START && nAddress <= CVpu::EE_ADDR_VU1AREA_END)
	{
		uint32 offset = nAddress - CVpu::EE_ADDR_VU1AREA_START;
		nReturn = HandleVu1AreaRead(offset);
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_gs != NULL)
		{
			nReturn = m_gs->ReadPrivRegister(nAddress);
		}
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled IO port (0x%08X, PC: 0x%08X).\r\n",
		                         nAddress, m_EE.m_State.nPC);
	}

	if((nAddress == CINTC::INTC_STAT) || (nAddress == CGSHandler::GS_CSR) || (nAddress == CTimer::T1_COUNT))
	{
		//Some games will loop checking for the vblank start interrupt or vblank event
		//This is usually a good sign indicating that the game is idling
		//Games sensitive to changes in the scheme:
		//- Tony Hawk Pro Skater 3
		//- Spongebob Squarepants: The Movie
		//- Gundam: Federation vs. Zeon
		//- Castlevania: Curse of Darkness
		//Games also need to catch the interrupt flag before the interrupt is serviced,
		//extra care is needed in NotifyVBlankStart not to break this

		//Only consider user code (non-kernel code) for idle loop detection
		if(m_EE.m_State.nPC < PS2::EE_RAM_SIZE)
		{
			static const uint32 checkCountMax = 5000;
			uint32& checkCount = m_statusRegisterCheckers[m_EE.m_State.nPC];
			checkCount = std::min<uint32>(checkCount + 1, checkCountMax);
			if(checkCount == checkCountMax)
			{
				m_EE.m_State.nHasException = MIPS_EXCEPTION_IDLE;
			}
		}
	}

	return nReturn;
}

uint32 CSubSystem::IOPortWriteHandler(uint32 nAddress, uint32 nData)
{
	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		m_timer.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10002000 && nAddress <= 0x1000203F)
	{
		m_ipu.SetRegister(nAddress, nData);
		ExecuteIpu();
	}
	else if(nAddress >= CGIF::REGS_START && nAddress < CGIF::REGS_END)
	{
		m_gif.SetRegister(nAddress, nData);
	}
	else if(nAddress >= CVif::REGS0_START && nAddress < CVif::REGS0_END)
	{
		m_vpu0->GetVif().SetRegister(nAddress, nData);
	}
	else if(nAddress >= CVif::REGS1_START && nAddress < CVif::REGS1_END)
	{
		m_vpu1->GetVif().SetRegister(nAddress, nData);
	}
	else if(nAddress >= CVif::VIF0_FIFO_START && nAddress < CVif::VIF0_FIFO_END)
	{
		m_vpu0->GetVif().SetRegister(nAddress, nData);
	}
	else if(nAddress >= CVif::VIF1_FIFO_START && nAddress < CVif::VIF1_FIFO_END)
	{
		m_vpu1->GetVif().SetRegister(nAddress, nData);
	}
	else if(nAddress >= CGIF::GIF_FIFO_START && nAddress < CGIF::GIF_FIFO_END)
	{
		m_gif.SetRegister(nAddress, nData);
	}
	else if(nAddress >= 0x10007000 && nAddress <= 0x1000702F)
	{
		m_ipu.SetRegister(nAddress, nData);
		ExecuteIpu();
	}
	else if(nAddress >= 0x10008000 && nAddress <= 0x1000EFFC)
	{
		m_dmac.SetRegister(nAddress, nData);
		ExecuteIpu();
	}
	else if(nAddress >= 0x1000F000 && nAddress <= 0x1000F01C)
	{
		m_intc.SetRegister(nAddress, nData);
	}
	else if(nAddress == 0x1000F180)
	{
		//stdout data
		m_iopBios.GetIoman()->Write(Iop::CIoman::FID_STDOUT, 1, &nData);
	}
	else if(nAddress >= 0x1000F520 && nAddress <= 0x1000F59C)
	{
		m_dmac.SetRegister(nAddress, nData);
	}
	else if(nAddress >= CVpu::EE_ADDR_VU1AREA_START && nAddress <= CVpu::EE_ADDR_VU1AREA_END)
	{
		uint32 offset = nAddress - CVpu::EE_ADDR_VU1AREA_START;
		HandleVu1AreaWrite(offset, nData);
	}
	else if(nAddress == CVpu::EE_ADDR_VU_FBRST)
	{
		m_vpu1->SetFbrst((nData >> 8) & 0xF);
	}
	else if(nAddress == CVpu::EE_ADDR_VU_CMSAR1)
	{
		bool validAddress = (nData & 0x7) == 0;
		if(validAddress)
		{
			m_vpu1->ExecuteMicroProgram(nData);
		}
	}
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_gs != NULL)
		{
			m_gs->WritePrivRegister(nAddress, nData);
		}
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Wrote to an unhandled IO port (0x%08X, 0x%08X, PC: 0x%08X).\r\n",
		                         nAddress, nData, m_EE.m_State.nPC);
	}

	bool isInterruptPending = m_intc.IsInterruptPending() || m_dmac.IsInterruptPending();
	if(
	    isInterruptPending &&
	    (m_EE.m_State.nHasException == MIPS_EXCEPTION_NONE) &&
	    ((m_EE.m_State.nCOP0[CCOP_SCU::STATUS] & INTERRUPTS_ENABLED_MASK) == INTERRUPTS_ENABLED_MASK))
	{
		m_EE.m_State.nHasException = MIPS_EXCEPTION_CHECKPENDINGINT;
	}

	return 0;
}

uint32 CSubSystem::Vu0MicroMemWriteHandler(uint32 address, uint32 value)
{
	uint32 baseAddress = (address - PS2::MICROMEM0ADDR) & ~0x03;
	*reinterpret_cast<uint32*>(m_microMem0 + baseAddress) = value;
	m_vpu0->InvalidateMicroProgram(baseAddress, baseAddress + 4);
	return 0;
}

uint32 CSubSystem::Vu0IoPortReadHandler(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case CVpu::VU_ADDR_ITOP:
		result = m_vpu0->GetVif().GetITOP();
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled VU0 IO port (0x%08X).\r\n", address);
		break;
	}
	return result;
}

uint32 CSubSystem::Vu0IoPortWriteHandler(uint32 address, uint32 value)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote an unhandled VU0 IO port (0x%08X, 0x%08X).\r\n",
		                         address, value);
		break;
	}
	return 0;
}

void CSubSystem::Vu0StateChanged(CVpu::VU_STATE newState)
{
	auto flushPipelines =
	    [](CMIPS& ctx) {
		    ctx.m_State.pipeQ.counter = 0;
		    ctx.m_State.nCOP2Q = ctx.m_State.pipeQ.heldValue;

		    VUShared::CheckFlagPipelineImmediate(VUShared::g_pipeInfoMac, &ctx, VUShared::LATENCY_MAC);
		    VUShared::CheckFlagPipelineImmediate(VUShared::g_pipeInfoSticky, &ctx, VUShared::LATENCY_MAC);
		    VUShared::CheckFlagPipelineImmediate(VUShared::g_pipeInfoClip, &ctx, VUShared::LATENCY_MAC);
	    };

	assert(newState != CVpu::VU_STATE_STOPPED);
	if(newState == CVpu::VU_STATE_RUNNING)
	{
		flushPipelines(m_EE);
		CopyVuState(m_VU0, m_EE);
	}
	else
	{
		flushPipelines(m_VU0);
		CopyVuState(m_EE, m_VU0);
	}
}

uint32 CSubSystem::Vu1MicroMemWriteHandler(uint32 address, uint32 value)
{
	uint32 baseAddress = (address - PS2::MICROMEM1ADDR) & ~0x03;
	*reinterpret_cast<uint32*>(m_microMem1 + baseAddress) = value;
	m_vpu1->InvalidateMicroProgram(baseAddress, baseAddress + 4);
	return 0;
}

uint32 CSubSystem::Vu1IoPortReadHandler(uint32 address)
{
	uint32 result = 0xCCCCCCCC;
	switch(address)
	{
	case CVpu::VU_ADDR_ITOP:
		result = m_vpu1->GetVif().GetITOP();
		break;
	case CVpu::VU_ADDR_TOP:
		result = m_vpu1->GetVif().GetTOP();
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled VU1 IO port (0x%08X).\r\n", address);
		break;
	}
	return result;
}

uint32 CSubSystem::Vu1IoPortWriteHandler(uint32 address, uint32 value)
{
	switch(address)
	{
	case CVpu::VU_ADDR_XGKICK:
		m_vpu1->ProcessXgKick(value);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote an unhandled VU1 IO port (0x%08X, 0x%08X).\r\n",
		                         address, value);
		break;
	}
	return 0;
}

void CSubSystem::CopyVuState(CMIPS& dst, const CMIPS& src)
{
	memcpy(&dst.m_State.nCOP2, &src.m_State.nCOP2, sizeof(dst.m_State.nCOP2));
	memcpy(&dst.m_State.nCOP2A, &src.m_State.nCOP2A, sizeof(dst.m_State.nCOP2A));
	memcpy(&dst.m_State.nCOP2VI, &src.m_State.nCOP2VI, sizeof(dst.m_State.nCOP2VI));
	dst.m_State.nCOP2Q = src.m_State.nCOP2Q;
	dst.m_State.nCOP2I = src.m_State.nCOP2I;
	dst.m_State.nCOP2R = src.m_State.nCOP2R;
	dst.m_State.nCOP2MF = src.m_State.nCOP2MF;
	dst.m_State.nCOP2SF = src.m_State.nCOP2SF;
	dst.m_State.nCOP2CF = src.m_State.nCOP2CF;
	dst.m_State.pipeQ.counter = 0;
	dst.m_State.pipeQ.heldValue = src.m_State.nCOP2Q;
	VUShared::ResetFlagPipelineImmediate(VUShared::g_pipeInfoMac, &dst, src.m_State.nCOP2MF);
	VUShared::ResetFlagPipelineImmediate(VUShared::g_pipeInfoSticky, &dst, src.m_State.nCOP2SF);
	VUShared::ResetFlagPipelineImmediate(VUShared::g_pipeInfoClip, &dst, src.m_State.nCOP2CF);
	dst.m_State.pipeMac.index = src.m_State.pipeMac.index;
	dst.m_State.pipeSticky.index = src.m_State.pipeSticky.index;
	dst.m_State.pipeClip.index = src.m_State.pipeClip.index;
}

uint32 CSubSystem::HandleVu1AreaRead(uint32 offset)
{
	assert(!m_vpu1->IsVuRunning());
	assert(offset < 0x400);
	uint32 result = 0;
	switch(offset)
	{
	case 0x3A0:
		result = m_vpu1->GetContext().m_State.nPC;
		break;
	case 0x3A4:
	case 0x3A8:
	case 0x3AC:
		break;
	default:
		assert(false);
		break;
	}
	return result;
}

void CSubSystem::HandleVu1AreaWrite(uint32 offset, uint32 value)
{
	assert(!m_vpu1->IsVuRunning());
	assert(offset < 0x400);
	if(offset >= 0 && offset <= 0x1FF)
	{
		uint32 registerIndex = offset / 0x10;
		uint32 elementIndex = (offset & 0x0F) / 4;
		if(registerIndex != 0)
		{
			m_VU1.m_State.nCOP2[registerIndex].nV[elementIndex] = value;
		}
	}
	else
	{
		assert(false);
	}
}

void CSubSystem::ExecuteIpu()
{
	m_dmac.ResumeDMA4();
	while(m_ipu.WillExecuteCommand())
	{
		m_ipu.ExecuteCommand();
		if(m_ipu.IsCommandDelayed())
		{
			break;
		}
		if(m_ipu.HasPendingOUTFIFOData())
		{
			break;
		}
		if(m_ipu.WillExecuteCommand() && m_dmac.IsDMA4Started())
		{
			m_dmac.ResumeDMA4();
		}
		else
		{
			break;
		}
	}
	if(m_ipu.HasPendingOUTFIFOData())
	{
		m_ipu.FlushOUTFIFOData();
	}
}

void CSubSystem::CheckPendingInterrupts()
{
	if(!m_EE.m_State.nHasException)
	{
		int32 cpuIntLine = -1;
		if(m_intc.IsInterruptPending())
		{
			cpuIntLine = 0;
		}
		else if(m_dmac.IsInterruptPending())
		{
			cpuIntLine = 1;
		}
		if(
		    (cpuIntLine != -1)
#ifdef DEBUGGER_INCLUDED
		    //			&& !m_singleStepEe
		    && !m_EE.m_executor->MustBreak()
#endif
		)
		{
			m_os->HandleInterrupt(cpuIntLine);
		}
	}
}

void CSubSystem::FlushInstructionCache()
{
	m_EE.m_executor->Reset();
}

void CSubSystem::LoadBIOS()
{
	auto biosPath = CAppConfig::GetInstance().GetBasePath() / "bios/scph10000.bin";
	auto biosStream = Framework::CreateInputStdStream(biosPath.native());
	biosStream.Read(m_bios, PS2::EE_BIOS_SIZE);
}

void CSubSystem::FillFakeIopRam()
{
	struct IOPMODINFO
	{
		uint32 nextPtr;
		uint32 namePtr;

		uint16 version;
		uint16 newFlags;
		uint16 id;
		uint16 flags;

		uint32 entry;
		uint32 gp;
		uint32 textStart;
		uint32 textSize;
		uint32 dataSize;
		uint32 bssSize;
		uint32 unused1;
		uint32 unused2;
	};

	IOPMODINFO* moduleInfo = reinterpret_cast<IOPMODINFO*>(m_fakeIopRam + 0x800);
	moduleInfo->nextPtr = 0x0;
	moduleInfo->namePtr = 0xC00;

	strcpy(reinterpret_cast<char*>(m_fakeIopRam + 0xC00), "sio2man");
}
