#include "Iop_SubSystem.h"
#include "../MemoryStateFile.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "placeholder_def.h"

using namespace Iop;
using namespace PS2;

#define LOG_NAME			("iop_subsystem")

#define STATE_CPU			("iop_cpu")
#define STATE_RAM			("iop_ram")
#define STATE_SCRATCH		("iop_scratch")
#define STATE_SPURAM		("iop_spuram")

CSubSystem::CSubSystem(bool ps2Mode) 
: m_cpu(MEMORYMAP_ENDIAN_LSBF)
, m_executor(m_cpu, (IOP_RAM_SIZE * 4))
, m_ram(new uint8[IOP_RAM_SIZE])
, m_scratchPad(new uint8[IOP_SCRATCH_SIZE])
, m_spuRam(new uint8[SPU_RAM_SIZE])
, m_dmac(m_ram, m_intc)
, m_counters(ps2Mode ? IOP_CLOCK_OVER_FREQ : IOP_CLOCK_BASE_FREQ, m_intc)
, m_spuCore0(m_spuRam, SPU_RAM_SIZE, 0)
, m_spuCore1(m_spuRam, SPU_RAM_SIZE, 1)
, m_spu(m_spuCore0)
, m_spu2(m_spuCore0, m_spuCore1)
#ifdef _IOP_EMULATE_MODULES
, m_sio2(m_intc)
#endif
, m_cpuArch(MIPS_REGSIZE_32)
, m_copScu(MIPS_REGSIZE_32)
, m_dmaUpdateTicks(0)
{
	//Read memory map
	m_cpu.m_pMemoryMap->InsertReadMap((0 * IOP_RAM_SIZE), (0 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,														0x01);
	m_cpu.m_pMemoryMap->InsertReadMap((1 * IOP_RAM_SIZE), (1 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,														0x02);
	m_cpu.m_pMemoryMap->InsertReadMap((2 * IOP_RAM_SIZE), (2 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,														0x03);
	m_cpu.m_pMemoryMap->InsertReadMap((3 * IOP_RAM_SIZE), (3 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,														0x04);
	m_cpu.m_pMemoryMap->InsertReadMap(0x1F800000,					0x1F8003FF,						m_scratchPad,												0x05);
	m_cpu.m_pMemoryMap->InsertReadMap(HW_REG_BEGIN,					HW_REG_END,						std::bind(&CSubSystem::ReadIoRegister, this, std::placeholders::_1),		0x06);

	//Write memory map
	m_cpu.m_pMemoryMap->InsertWriteMap((0 * IOP_RAM_SIZE),		(0 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,																			0x01);
	m_cpu.m_pMemoryMap->InsertWriteMap((1 * IOP_RAM_SIZE),		(1 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,																			0x02);
	m_cpu.m_pMemoryMap->InsertWriteMap((2 * IOP_RAM_SIZE),		(2 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,																			0x03);
	m_cpu.m_pMemoryMap->InsertWriteMap((3 * IOP_RAM_SIZE),		(3 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,																			0x04);
	m_cpu.m_pMemoryMap->InsertWriteMap(0x1F800000,		0x1F8003FF,									m_scratchPad,																	0x05);
	m_cpu.m_pMemoryMap->InsertWriteMap(HW_REG_BEGIN,	HW_REG_END,									std::bind(&CSubSystem::WriteIoRegister, this, std::placeholders::_1, std::placeholders::_2),		0x06);

	//Instruction memory map
	m_cpu.m_pMemoryMap->InsertInstructionMap((0 * IOP_RAM_SIZE), (0 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,						0x01);
	m_cpu.m_pMemoryMap->InsertInstructionMap((1 * IOP_RAM_SIZE), (1 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,						0x02);
	m_cpu.m_pMemoryMap->InsertInstructionMap((2 * IOP_RAM_SIZE), (2 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,						0x03);
	m_cpu.m_pMemoryMap->InsertInstructionMap((3 * IOP_RAM_SIZE), (3 * IOP_RAM_SIZE) + IOP_RAM_SIZE - 1,	m_ram,						0x04);

	m_cpu.m_pArch = &m_cpuArch;
	m_cpu.m_pCOP[0] = &m_copScu;
	m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;

	m_dmac.SetReceiveFunction(4, bind(&CSpuBase::ReceiveDma, &m_spuCore0, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3));
	m_dmac.SetReceiveFunction(8, bind(&CSpuBase::ReceiveDma, &m_spuCore1, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3));
}

CSubSystem::~CSubSystem()
{
	m_bios.reset();
	delete [] m_ram;
	delete [] m_scratchPad;
	delete [] m_spuRam;
}

void CSubSystem::SetBios(const BiosBasePtr& bios)
{
	m_bios = bios;
}

void CSubSystem::NotifyVBlankStart()
{
	m_bios->NotifyVBlankStart();
	m_intc.AssertLine(Iop::CIntc::LINE_VBLANK);
}

void CSubSystem::NotifyVBlankEnd()
{
	m_bios->NotifyVBlankEnd();
	m_intc.AssertLine(Iop::CIntc::LINE_EVBLANK);
}

void CSubSystem::SaveState(Framework::CZipArchiveWriter& archive)
{
	archive.InsertFile(new CMemoryStateFile(STATE_CPU,		&m_cpu.m_State, sizeof(MIPSSTATE)));
	archive.InsertFile(new CMemoryStateFile(STATE_RAM,		m_ram,			IOP_RAM_SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_SCRATCH,	m_scratchPad,	IOP_SCRATCH_SIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_SPURAM,	m_spuRam,		SPU_RAM_SIZE));
	m_intc.SaveState(archive);
	m_counters.SaveState(archive);
	m_spuCore0.SaveState(archive);
	m_spuCore1.SaveState(archive);
	m_bios->SaveState(archive);
}

void CSubSystem::LoadState(Framework::CZipArchiveReader& archive)
{
	archive.BeginReadFile(STATE_CPU			)->Read(&m_cpu.m_State,	sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_RAM			)->Read(m_ram,			IOP_RAM_SIZE);
	archive.BeginReadFile(STATE_SCRATCH		)->Read(m_scratchPad,	IOP_SCRATCH_SIZE);
	archive.BeginReadFile(STATE_SPURAM		)->Read(m_spuRam,		SPU_RAM_SIZE);
	m_intc.LoadState(archive);
	m_counters.LoadState(archive);
	m_spuCore0.LoadState(archive);
	m_spuCore1.LoadState(archive);
	m_bios->LoadState(archive);
}

void CSubSystem::Reset()
{
	memset(m_ram, 0, IOP_RAM_SIZE);
	memset(m_scratchPad, 0, IOP_SCRATCH_SIZE);
	memset(m_spuRam, 0, SPU_RAM_SIZE);
	m_executor.Reset();
	m_cpu.Reset();
	m_cpu.m_analysis->Clear();
	m_spuCore0.Reset();
	m_spuCore1.Reset();
	m_spu.Reset();
	m_spu2.Reset();
#ifdef _IOP_EMULATE_MODULES
	m_sio2.Reset();
#endif
	m_counters.Reset();
	m_dmac.Reset();
	m_intc.Reset();
	m_bios.reset();

	m_cpu.m_Comments.RemoveTags();
	m_cpu.m_Functions.RemoveTags();

	m_dmaUpdateTicks = 0;
}

uint32 CSubSystem::ReadIoRegister(uint32 address)
{
	if(address == 0x1F801814)
	{
		return 0x14802000;
	}
	else if(address >= CSpu::SPU_BEGIN && address <= CSpu::SPU_END)
	{
		return m_spu.ReadRegister(address);
	}
	else if(address >= CDmac::DMAC_ZONE1_START && address <= CDmac::DMAC_ZONE1_END)
	{
		return m_dmac.ReadRegister(address);
	}
	else if(address >= CDmac::DMAC_ZONE2_START && address <= CDmac::DMAC_ZONE2_END)
	{
		return m_dmac.ReadRegister(address);
	}
	else if(address >= CIntc::ADDR_BEGIN && address <= CIntc::ADDR_END)
	{
		return m_intc.ReadRegister(address);
	}
	else if(
		(address >= CRootCounters::ADDR_BEGIN1 && address <= CRootCounters::ADDR_END1) ||
		(address >= CRootCounters::ADDR_BEGIN2 && address <= CRootCounters::ADDR_END2)
		)
	{
		return m_counters.ReadRegister(address);
	}
#ifdef _IOP_EMULATE_MODULES
	else if(address >= CSio2::ADDR_BEGIN && address <= CSio2::ADDR_END)
	{
		return m_sio2.ReadRegister(address);
	}
#endif
	else if(address >= CSpu2::REGS_BEGIN && address <= CSpu2::REGS_END)
	{
		return m_spu2.ReadRegister(address);
	}
	else if(address >= 0x1F808400 && address <= 0x1F808500)
	{
		//iLink (aka Firewire) stuff
		return 0x08;
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Reading an unknown hardware register (0x%0.8X).\r\n", address);
	}
	return 0;
}

uint32 CSubSystem::WriteIoRegister(uint32 address, uint32 value)
{
	if(address >= CDmac::DMAC_ZONE1_START && address <= CDmac::DMAC_ZONE1_END)
	{
		m_dmac.WriteRegister(address, value);
	}
	else if(address >= CSpu::SPU_BEGIN && address <= CSpu::SPU_END)
	{
		m_spu.WriteRegister(address, static_cast<uint16>(value));
	}
	else if(address >= CDmac::DMAC_ZONE2_START && address <= CDmac::DMAC_ZONE2_END)
	{
		m_dmac.WriteRegister(address, value);
	}
	else if(address >= CIntc::ADDR_BEGIN && address <= CIntc::ADDR_END)
	{
		m_intc.WriteRegister(address, value);
	}
	else if(
		(address >= CRootCounters::ADDR_BEGIN1 && address <= CRootCounters::ADDR_END1) ||
		(address >= CRootCounters::ADDR_BEGIN2 && address <= CRootCounters::ADDR_END2)
		)
	{
		m_counters.WriteRegister(address, value);
	}
#ifdef _IOP_EMULATE_MODULES
	else if(address >= CSio2::ADDR_BEGIN && address <= CSio2::ADDR_END)
	{
		m_sio2.WriteRegister(address, value);
	}
#endif
	else if(address >= CSpu2::REGS_BEGIN && address <= CSpu2::REGS_END)
	{
		return m_spu2.WriteRegister(address, value);
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Writing to an unknown hardware register (0x%0.8X, 0x%0.8X).\r\n", address, value);
	}
	return 0;
}

bool CSubSystem::IsCpuIdle()
{
	if(m_bios->IsIdle())
	{
		return true;
	}
	else
	{
		uint32 physicalPc = m_cpu.m_pAddrTranslator(&m_cpu, m_cpu.m_State.nPC);
		CBasicBlock* nextBlock = m_executor.FindBlockAt(physicalPc);
		if(nextBlock && nextBlock->GetSelfLoopCount() > 5000)
		{
			//Go a little bit faster if we're "stuck"
			return true;
		}
	}
	return false;
}

void CSubSystem::CountTicks(int ticks)
{
	static int g_dmaUpdateDelay = 10000;
	m_counters.Update(ticks);
	m_bios->CountTicks(ticks);
	m_dmaUpdateTicks += ticks;
	if(m_dmaUpdateTicks >= g_dmaUpdateDelay)
	{
		m_dmac.ResumeDma(4);
		m_dmac.ResumeDma(8);
		m_dmaUpdateTicks -= g_dmaUpdateDelay;
	}
	{
		bool irqPending = false;
		irqPending |= m_spuCore0.GetIrqPending();
		irqPending |= m_spuCore1.GetIrqPending();
		if(irqPending)
		{
			m_intc.AssertLine(CIntc::LINE_SPU2);
		}
		else
		{
			m_intc.ClearLine(CIntc::LINE_SPU2);
		}
	}
}

int CSubSystem::ExecuteCpu(int quota)
{
	int executed = 0;
	if(!m_cpu.m_State.nHasException)
	{
		if(m_intc.HasPendingInterrupt())
		{
			m_bios->HandleInterrupt();
		}
	}
	if(!m_cpu.m_State.nHasException)
	{
		executed = (quota - m_executor.Execute(quota));
	}
	if(m_cpu.m_State.nHasException)
	{
		m_bios->HandleException();
	}
	return executed;
}
