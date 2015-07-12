#include "Ee_SubSystem.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "../MemoryStateFile.h"
#include "../iop/IopBios.h"
#include "Vif.h"
#include "placeholder_def.h"

using namespace Ee;

#define LOG_NAME		("ee_subsystem")

#define STATE_EE		("ee")
#define STATE_VU0		("vu0")
#define STATE_VU1		("vu1")
#define STATE_RAM		("ram")
#define STATE_SPR		("spr")
#define STATE_VUMEM0	("vumem0")
#define STATE_MICROMEM0	("micromem0")
#define STATE_VUMEM1	("vumem1")
#define STATE_MICROMEM1	("micromem1")

#define FAKE_IOP_RAM_SIZE	(0x1000)

CSubSystem::CSubSystem(uint8* iopRam, CIopBios& iopBios)
: m_ram(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::EE_RAM_SIZE, framework_getpagesize())))
, m_bios(new uint8[PS2::EE_BIOS_SIZE])
, m_spr(new uint8[PS2::EE_SPR_SIZE])
, m_fakeIopRam(new uint8[FAKE_IOP_RAM_SIZE])
, m_vuMem0(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::VUMEM0SIZE, 0x10)))
, m_microMem0(new uint8[PS2::MICROMEM0SIZE])
, m_vuMem1(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::VUMEM1SIZE, 0x10)))
, m_microMem1(new uint8[PS2::MICROMEM1SIZE])
, m_EE(MEMORYMAP_ENDIAN_LSBF)
, m_VU0(MEMORYMAP_ENDIAN_LSBF)
, m_VU1(MEMORYMAP_ENDIAN_LSBF)
, m_executor(m_EE, m_ram)
, m_dmac(m_ram, m_spr, m_vuMem0, m_EE)
, m_gif(m_gs, m_ram, m_spr)
, m_sif(m_dmac, m_ram, iopRam)
, m_intc(m_dmac, m_gs)
, m_timer(m_intc)
, m_COP_SCU(MIPS_REGSIZE_64)
, m_COP_FPU(MIPS_REGSIZE_64)
, m_COP_VU(MIPS_REGSIZE_64)
, m_iopBios(iopBios)
{
	//Some alignment checks, this is needed because of SIMD instructions used in generated code
	assert((reinterpret_cast<size_t>(&m_EE.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(&m_VU0.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(&m_VU1.m_State) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(m_vuMem0) & 0x0F) == 0);
	assert((reinterpret_cast<size_t>(m_vuMem1) & 0x0F) == 0);

	m_vpu0 = std::make_shared<CVpu>(0, CVpu::VPUINIT(m_microMem0, m_vuMem0, &m_VU0), m_gif, m_ram, m_spr);
	m_vpu1 = std::make_shared<CVpu>(1, CVpu::VPUINIT(m_microMem1, m_vuMem1, &m_VU1), m_gif, m_ram, m_spr);

	//EmotionEngine context setup
	{
		//Read map
		m_EE.m_pMemoryMap->InsertReadMap(0x00000000,			0x01FFFFFF,									m_ram,														0x00);
		m_EE.m_pMemoryMap->InsertReadMap(0x02000000,			0x02003FFF,									m_spr,														0x01);
		m_EE.m_pMemoryMap->InsertReadMap(0x10000000,			0x10FFFFFF,									bind(&CSubSystem::IOPortReadHandler, this, PLACEHOLDER_1),	0x02);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::MICROMEM0ADDR,	PS2::MICROMEM0ADDR + PS2::MICROMEM0SIZE,	m_microMem0,												0x03);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::VUMEM0ADDR,		PS2::VUMEM0ADDR + PS2::VUMEM0SIZE,			m_vuMem0,													0x04);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::MICROMEM1ADDR,	PS2::MICROMEM1ADDR + PS2::MICROMEM1SIZE,	m_microMem1,												0x05);
		m_EE.m_pMemoryMap->InsertReadMap(PS2::VUMEM1ADDR,		PS2::VUMEM1ADDR + PS2::VUMEM1SIZE,			m_vuMem1,													0x06);
		m_EE.m_pMemoryMap->InsertReadMap(0x12000000,			0x12FFFFFF,									bind(&CSubSystem::IOPortReadHandler, this, PLACEHOLDER_1),	0x07);
		m_EE.m_pMemoryMap->InsertReadMap(0x1C000000,			0x1C001000,									m_fakeIopRam,												0x08);
		m_EE.m_pMemoryMap->InsertReadMap(0x1FC00000,			0x1FFFFFFF,									m_bios,														0x09);

		//Write map
		m_EE.m_pMemoryMap->InsertWriteMap(0x00000000,			0x01FFFFFF,									m_ram,																			0x00);
		m_EE.m_pMemoryMap->InsertWriteMap(0x02000000,			0x02003FFF,									m_spr,																			0x01);
		m_EE.m_pMemoryMap->InsertWriteMap(0x10000000,			0x10FFFFFF,									bind(&CSubSystem::IOPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),		0x02);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::MICROMEM0ADDR,	PS2::MICROMEM0ADDR + PS2::MICROMEM0SIZE,	bind(&CSubSystem::Vu0MicroMemWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),	0x03);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::VUMEM0ADDR,		PS2::VUMEM0ADDR + PS2::VUMEM0SIZE,			m_vuMem0,																		0x04);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::MICROMEM1ADDR,	PS2::MICROMEM1ADDR + PS2::MICROMEM1SIZE,	m_microMem1,																	0x05);
		m_EE.m_pMemoryMap->InsertWriteMap(PS2::VUMEM1ADDR,		PS2::VUMEM1ADDR + PS2::VUMEM1SIZE,			m_vuMem1,																		0x06);
		m_EE.m_pMemoryMap->InsertWriteMap(0x12000000,			0x12FFFFFF,									bind(&CSubSystem::IOPortWriteHandler,	this, PLACEHOLDER_1, PLACEHOLDER_2),	0x07);

		//Instruction map
		m_EE.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x01FFFFFF, m_ram,	0x00);
		m_EE.m_pMemoryMap->InsertInstructionMap(0x1FC00000, 0x1FFFFFFF, m_bios,	0x01);

		m_EE.m_pArch			= &m_EEArch;
		m_EE.m_pCOP[0]			= &m_COP_SCU;
		m_EE.m_pCOP[1]			= &m_COP_FPU;
		m_EE.m_pCOP[2]			= &m_COP_VU;

		m_EE.m_pAddrTranslator	= CPS2OS::TranslateAddress;
	}

	//Vector Unit 0 context setup
	{
		m_VU0.m_pMemoryMap->InsertReadMap(0x00000000, 0x00000FFF, m_vuMem0,														0x01);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00001000, 0x00001FFF, m_vuMem0,														0x02);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00002000, 0x00002FFF, m_vuMem0,														0x03);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00003000, 0x00003FFF, m_vuMem0,														0x04);
		m_VU0.m_pMemoryMap->InsertReadMap(0x00004000, 0x00008FFF, bind(&CSubSystem::Vu0IoPortReadHandler, this, PLACEHOLDER_1),	0x05);

		m_VU0.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00000FFF, m_vuMem0,																		0x01);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00001000, 0x00001FFF, m_vuMem0,																		0x02);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00002000, 0x00002FFF, m_vuMem0,																		0x03);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00003000, 0x00003FFF, m_vuMem0,																		0x04);
		m_VU0.m_pMemoryMap->InsertWriteMap(0x00004000, 0x00008FFF, bind(&CSubSystem::Vu0IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),	0x05);

		m_VU0.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00000FFF, m_microMem0, 0x00);

		m_VU0.m_pArch			= &m_MAVU0;
		m_VU0.m_pAddrTranslator	= CMIPS::TranslateAddress64;
	}

	//Vector Unit 1 context setup
	{
		m_VU1.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_vuMem1,															0x00);
		m_VU1.m_pMemoryMap->InsertReadMap(0x00008000, 0x00008FFF, bind(&CSubSystem::Vu1IoPortReadHandler, this, PLACEHOLDER_1),		0x01);

		m_VU1.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_vuMem1,																		0x00);
		m_VU1.m_pMemoryMap->InsertWriteMap(0x00008000, 0x00008FFF, bind(&CSubSystem::Vu1IoPortWriteHandler, this, PLACEHOLDER_1, PLACEHOLDER_2),	0x01);

		m_VU1.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00003FFF, m_microMem1, 0x01);

		m_VU1.m_pArch			= &m_MAVU1;
		m_VU1.m_pAddrTranslator	= CMIPS::TranslateAddress64;
	}

	m_EE.m_vuMem = m_vuMem0;
	m_VU0.m_vuMem = m_vuMem0;
	m_VU1.m_vuMem = m_vuMem1;

	m_dmac.SetChannelTransferFunction(0, bind(&CVif::ReceiveDMA, &m_vpu0->GetVif(), PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(1, bind(&CVif::ReceiveDMA, &m_vpu1->GetVif(), PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(2, bind(&CGIF::ReceiveDMA, &m_gif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(4, bind(&CIPU::ReceiveDMA4, &m_ipu, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_4, m_ram));
	m_dmac.SetChannelTransferFunction(5, bind(&CSIF::ReceiveDMA5, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));
	m_dmac.SetChannelTransferFunction(6, bind(&CSIF::ReceiveDMA6, &m_sif, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4));

	m_ipu.SetDMA3ReceiveHandler(bind(&CDMAC::ResumeDMA3, &m_dmac, PLACEHOLDER_1, PLACEHOLDER_2));

	m_os = new CPS2OS(m_EE, m_ram, m_bios, m_spr, m_gs, m_sif, iopBios);
	m_os->OnRequestInstructionCacheFlush.connect(boost::bind(&CSubSystem::FlushInstructionCache, this));
}

CSubSystem::~CSubSystem()
{
	m_executor.Reset();
	framework_aligned_free(m_ram);
	delete [] m_bios;
	delete [] m_fakeIopRam;
	framework_aligned_free(m_vuMem0);
	framework_aligned_free(m_vuMem1);
	delete m_os;
}

void CSubSystem::SetVpu0(std::shared_ptr<CVpu> newVpu0)
{
	m_vpu0 = newVpu0;
}

void CSubSystem::SetVpu1(std::shared_ptr<CVpu> newVpu1)
{
	m_vpu1 = newVpu1;
}

void CSubSystem::Reset()
{
	m_os->Release();
	m_executor.Reset();

	memset(m_ram,			0, PS2::EE_RAM_SIZE);
	memset(m_spr,			0, PS2::EE_SPR_SIZE);
	memset(m_bios,			0, PS2::EE_BIOS_SIZE);
	memset(m_fakeIopRam,	0, FAKE_IOP_RAM_SIZE);
	memset(m_vuMem0,		0, PS2::VUMEM0SIZE);
	memset(m_microMem0,		0, PS2::MICROMEM0SIZE);
	memset(m_vuMem1,		0, PS2::VUMEM1SIZE);
	memset(m_microMem1,		0, PS2::MICROMEM1SIZE);

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

	m_os->Initialize();
	FillFakeIopRam();

	m_statusRegisterCheckers.clear();
	m_isIdle = false;
}

int CSubSystem::ExecuteCpu(int quota)
{
	m_isIdle = false;
	int executed = 0;
	if(m_EE.m_State.callMsEnabled)
	{
		if(!m_vpu0->IsVuRunning())
		{
			//callMs mode over
			memcpy(&m_EE.m_State.nCOP2,		&m_VU0.m_State.nCOP2,	sizeof(m_EE.m_State.nCOP2));
			memcpy(&m_EE.m_State.nCOP2A,	&m_VU0.m_State.nCOP2A,	sizeof(m_EE.m_State.nCOP2A));
			memcpy(&m_EE.m_State.nCOP2VI,	&m_VU0.m_State.nCOP2VI, sizeof(m_EE.m_State.nCOP2VI));
			m_EE.m_State.callMsEnabled = 0;
		}
	}
	else if(!m_EE.m_State.nHasException)
	{
		executed = (quota - m_executor.Execute(quota));
	}
	if(m_EE.m_State.nHasException)
	{
		switch(m_EE.m_State.nHasException)
		{
		case MIPS_EXCEPTION_SYSCALL:
			m_os->HandleSyscall();
			break;
		case MIPS_EXCEPTION_CALLMS:
			assert(m_EE.m_State.callMsEnabled);
			if(m_EE.m_State.callMsEnabled)
			{
				//We are in callMs mode
				assert(!m_vpu0->IsVuRunning());
				//Copy the COP2 state to VPU0
				memcpy(&m_VU0.m_State.nCOP2,	&m_EE.m_State.nCOP2,	sizeof(m_VU0.m_State.nCOP2));
				memcpy(&m_VU0.m_State.nCOP2A,	&m_EE.m_State.nCOP2A,	sizeof(m_VU0.m_State.nCOP2A));
				memcpy(&m_VU0.m_State.nCOP2VI,	&m_EE.m_State.nCOP2VI,	sizeof(m_VU0.m_State.nCOP2VI));
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
	CBasicBlock* nextBlock = m_executor.FindBlockAt(m_EE.m_State.nPC);
	if(nextBlock && nextBlock->GetSelfLoopCount() > 5000)
	{
		return true;
	}
	else if(m_os->IsIdle() || m_isIdle)
	{
		return true;
	}
	else if(m_EE.m_State.nPC >= 0x1FC03100 && m_EE.m_State.nPC <= 0x1FC03110)
	{
		return true;
	}
	return false;
}

void CSubSystem::CountTicks(int ticks)
{
	if(!m_vpu0->IsVuRunning() || (m_vpu0->IsVuRunning() && !m_vpu0->GetVif().IsWaitingForProgramEnd()))
	{
		m_dmac.ResumeDMA0();
	}
	if(!m_vpu1->IsVuRunning() || (m_vpu1->IsVuRunning() && !m_vpu1->GetVif().IsWaitingForProgramEnd()))
	{
		m_dmac.ResumeDMA1();
		if(m_vpu1->GetVif().IsStalledByInterrupt())
		{
			m_intc.AssertLine(CINTC::INTC_LINE_VIF1);
		}
	}
	m_dmac.ResumeDMA8();
	ExecuteIpu();
	if(!m_EE.m_State.nHasException)
	{
		if((m_EE.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0)
		{
			m_sif.ProcessPackets();
		}
	}
	CheckPendingInterrupts();
	m_EE.m_State.nCOP0[CCOP_SCU::COUNT] += ticks;
	m_timer.Count(ticks);
}

void CSubSystem::NotifyVBlankStart()
{
	m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_START);
	if(m_os->CheckVBlankFlag())
	{
		//Make sure a vblank start interrupt is serviced now because
		//if vsync flag was set, we want to make sure interrupt is caught
		CheckPendingInterrupts();
	}
}

void CSubSystem::NotifyVBlankEnd()
{
	m_intc.AssertLine(CINTC::INTC_LINE_VBLANK_END);
}

void CSubSystem::SaveState(Framework::CZipArchiveWriter& archive)
{
	archive.InsertFile(new CMemoryStateFile(STATE_EE,			&m_EE.m_State,	sizeof(MIPSSTATE)));
	archive.InsertFile(new CMemoryStateFile(STATE_VU0,			&m_VU0.m_State,	sizeof(MIPSSTATE)));
	archive.InsertFile(new CMemoryStateFile(STATE_VU1,			&m_VU1.m_State,	sizeof(MIPSSTATE)));
	archive.InsertFile(new CMemoryStateFile(STATE_RAM,			m_ram,			PS2::EE_RAM_SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_SPR,			m_spr,			PS2::EE_SPR_SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_VUMEM0,		m_vuMem0,		PS2::VUMEM0SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_MICROMEM0,	m_microMem0,	PS2::MICROMEM0SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_VUMEM1,		m_vuMem1,		PS2::VUMEM1SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_MICROMEM1,	m_microMem1,	PS2::MICROMEM1SIZE));

	m_dmac.SaveState(archive);
	m_intc.SaveState(archive);
	m_sif.SaveState(archive);
	m_vpu0->SaveState(archive);
	m_vpu1->SaveState(archive);
	m_timer.SaveState(archive);
}

void CSubSystem::LoadState(Framework::CZipArchiveReader& archive)
{
	archive.BeginReadFile(STATE_EE			)->Read(&m_EE.m_State,	sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_VU0			)->Read(&m_VU0.m_State,	sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_VU1			)->Read(&m_VU1.m_State,	sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_RAM			)->Read(m_ram,			PS2::EE_RAM_SIZE);
	archive.BeginReadFile(STATE_SPR			)->Read(m_spr,			PS2::EE_SPR_SIZE);
	archive.BeginReadFile(STATE_VUMEM0		)->Read(m_vuMem0,		PS2::VUMEM0SIZE);
	archive.BeginReadFile(STATE_MICROMEM0	)->Read(m_microMem0,	PS2::MICROMEM0SIZE);
	archive.BeginReadFile(STATE_VUMEM1		)->Read(m_vuMem1,		PS2::VUMEM1SIZE);
	archive.BeginReadFile(STATE_MICROMEM1	)->Read(m_microMem1,	PS2::MICROMEM1SIZE);

	m_dmac.LoadState(archive);
	m_intc.LoadState(archive);
	m_sif.LoadState(archive);
	m_vpu0->LoadState(archive);
	m_vpu1->LoadState(archive);
	m_timer.LoadState(archive);

	m_executor.Reset();
}

uint32 CSubSystem::IOPortReadHandler(uint32 nAddress)
{
	uint32 nReturn = 0;
	if(nAddress >= 0x10000000 && nAddress <= 0x1000183F)
	{
		nReturn = m_timer.GetRegister(nAddress);
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
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_gs != NULL)
		{
			nReturn = m_gs->ReadPrivRegister(nAddress);
		}
	}
	else
	{
		printf("PS2VM: Read an unhandled IO port (0x%0.8X).\r\n", nAddress);
	}

	if((nAddress == CINTC::INTC_STAT) || (nAddress == CGSHandler::GS_CSR))
	{
		static const uint32 checkCountMax = 5000;
		uint32& checkCount = m_statusRegisterCheckers[m_EE.m_State.nPC];
		checkCount = std::min<uint32>(checkCount + 1, checkCountMax);
		if(checkCount == checkCountMax)
		{
			m_EE.m_State.nHasException = MIPS_EXCEPTION_IDLE;
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
	else if(nAddress >= 0x12000000 && nAddress <= 0x1200108C)
	{
		if(m_gs != NULL)
		{
			m_gs->WritePrivRegister(nAddress, nData);
		}
	}
	else
	{
		printf("PS2VM: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nData, m_EE.m_State.nPC);
	}

	return 0;
}

uint32 CSubSystem::Vu0MicroMemWriteHandler(uint32 address, uint32 value)
{
	*reinterpret_cast<uint32*>(m_microMem0 + (address - PS2::MICROMEM0ADDR)) = value;
	m_vpu0->InvalidateMicroProgram();
	return 0;
}

uint32 CSubSystem::Vu0IoPortReadHandler(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case CVpu::VU_ITOP:
		result = m_vpu0->GetVif().GetITOP();
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled VU0 IO port (0x%0.8X).\r\n", address);
		break;
	}
	return result;
}

uint32 CSubSystem::Vu0IoPortWriteHandler(uint32 address, uint32 value)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote an unhandled VU0 IO port (0x%0.8X, 0x%0.8X).\r\n", 
								  address, value);
		break;
	}
	return 0;
}

uint32 CSubSystem::Vu1IoPortReadHandler(uint32 address)
{
	uint32 result = 0xCCCCCCCC;
	switch(address)
	{
	case CVpu::VU_ITOP:
		result = m_vpu1->GetVif().GetITOP();
		break;
	case CVpu::VU_TOP:
		result = m_vpu1->GetVif().GetTOP();
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled VU1 IO port (0x%0.8X).\r\n", address);
		break;
	}
	return result;
}

uint32 CSubSystem::Vu1IoPortWriteHandler(uint32 address, uint32 value)
{
	switch(address)
	{
	case CVpu::VU_XGKICK:
		m_vpu1->ProcessXgKick(value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote an unhandled VU1 IO port (0x%0.8X, 0x%0.8X).\r\n", 
								  address, value);
		break;
	}
	return 0;
}

void CSubSystem::ExecuteIpu()
{
	m_dmac.ResumeDMA4();
	while(m_ipu.WillExecuteCommand())
	{
		m_ipu.ExecuteCommand();
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
}

void CSubSystem::CheckPendingInterrupts()
{
	if(!m_EE.m_State.nHasException)
	{
		if(
			m_intc.IsInterruptPending()
#ifdef DEBUGGER_INCLUDED
//			&& !m_singleStepEe
			&& !m_executor.MustBreak()
#endif
			)
		{
			m_os->HandleInterrupt();
		}
	}
}

void CSubSystem::FlushInstructionCache()
{
	m_executor.Reset();
}

void CSubSystem::LoadBIOS()
{
	Framework::CStdStream BiosStream(fopen("./vfs/rom0/scph10000.bin", "rb"));
	BiosStream.Read(m_bios, PS2::EE_BIOS_SIZE);
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
