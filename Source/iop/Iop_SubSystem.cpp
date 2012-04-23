#include "Iop_SubSystem.h"
#include "../MemoryStateFile.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "placeholder_def.h"

using namespace Iop;
using namespace PS2;

#define LOG_NAME ("iop_subsystem")

#define STATE_CPU		("iop_cpu")
#define STATE_RAM		("iop_ram")
#define STATE_SCRATCH	("iop_scratch")

CSubSystem::CSubSystem() 
: m_cpu(MEMORYMAP_ENDIAN_LSBF)
, m_executor(m_cpu, (IOP_RAM_SIZE * 4))
, m_ram(new uint8[IOP_RAM_SIZE])
, m_scratchPad(new uint8[IOP_SCRATCH_SIZE])
, m_spuRam(new uint8[SPU_RAM_SIZE])
, m_dmac(m_ram, m_intc)
, m_counters(IOP_CLOCK_FREQ, m_intc)
, m_spuCore0(m_spuRam, SPU_RAM_SIZE)
, m_spuCore1(m_spuRam, SPU_RAM_SIZE)
, m_spu(m_spuCore0)
, m_spu2(m_spuCore0, m_spuCore1)
, m_cpuArch(MIPS_REGSIZE_32)
, m_copScu(MIPS_REGSIZE_32)
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
	delete [] m_ram;
	delete [] m_scratchPad;
	delete [] m_spuRam;
}

void CSubSystem::SetBios(const BiosPtr& bios)
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
	m_bios->SaveState(archive);
}

void CSubSystem::LoadState(Framework::CZipArchiveReader& archive)
{
	archive.BeginReadFile(STATE_CPU			)->Read(&m_cpu.m_State,	sizeof(MIPSSTATE));
	archive.BeginReadFile(STATE_RAM			)->Read(m_ram,			IOP_RAM_SIZE);
	archive.BeginReadFile(STATE_SCRATCH		)->Read(m_scratchPad,	IOP_SCRATCH_SIZE);
	m_bios->LoadState(archive);
}

void CSubSystem::Reset()
{
	memset(m_ram, 0, IOP_RAM_SIZE);
	memset(m_scratchPad, 0, IOP_SCRATCH_SIZE);
	memset(m_spuRam, 0, SPU_RAM_SIZE);
	m_executor.Reset();
	m_cpu.Reset();
	m_cpu.m_pAnalysis->Clear();
	m_spuCore0.Reset();
	m_spuCore1.Reset();
	m_spu.Reset();
	m_spu2.Reset();
	m_counters.Reset();
	m_dmac.Reset();
	m_intc.Reset();
	m_bios.reset();

	m_cpu.m_Comments.RemoveTags();
	m_cpu.m_Functions.RemoveTags();
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
	else if(address >= CRootCounters::ADDR_BEGIN && address <= CRootCounters::ADDR_END)
	{
		return m_counters.ReadRegister(address);
	}
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
	else if(address >= CRootCounters::ADDR_BEGIN && address <= CRootCounters::ADDR_END)
	{
		m_counters.WriteRegister(address, value);
	}
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

unsigned int CSubSystem::ExecuteCpu(bool singleStep)
{
	int ticks = 0;
	if(!m_cpu.m_State.nHasException)
	{
		if(m_intc.HasPendingInterrupt())
		{
			m_bios->HandleInterrupt();
		}
	}
	if(!m_cpu.m_State.nHasException)
	{
		bool isIdle = false;
		int quota = singleStep ? 1 : 500;

		{
			if(m_bios->IsIdle())
			{
				isIdle = true;
				ticks += (quota * 2);
			}
			else
			{
				uint32 physicalPc = m_cpu.m_pAddrTranslator(&m_cpu, m_cpu.m_State.nPC);
				CBasicBlock* nextBlock = m_executor.FindBlockAt(physicalPc);
				if(nextBlock && nextBlock->GetSelfLoopCount() > 5000)
				{
					//Go a little bit faster if we're "stuck"
					isIdle = true;
					ticks += (quota * 2);
				}
			}
		}

		if(isIdle && !singleStep)
		{
			quota /= 50;
		}

		ticks += (quota - m_executor.Execute(quota));
		assert(ticks >= 0);

		if(ticks > 0)
		{
			m_counters.Update(ticks);
			m_bios->CountTicks(ticks);
		}
	}
	if(m_cpu.m_State.nHasException)
	{
		m_bios->HandleException();
	}
	return ticks;
}
