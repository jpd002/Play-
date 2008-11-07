#include <stdexcept>
#include <assert.h>
#include "PsfVm.h"
#include "Log.h"
#include "MA_MIPSIV.h"
#include "HighResTimer.h"

#define LOG_NAME ("psfvm")

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;
using namespace Iop;

#define CLOCK_FREQ		(44100 * 256 * 3)		//~33.8MHz
#define FRAMES_PER_SEC	(60)

const int g_frameTicks = (CLOCK_FREQ / FRAMES_PER_SEC);
const int g_spuUpdateTicks = (4 * CLOCK_FREQ / 1000);

CPsfVm::CPsfVm() :
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0, 0x1FFFFFFF),
m_executor(m_cpu),
m_status(PAUSED),
m_bios(NULL),
m_singleStep(false),
m_ram(new uint8[RAMSIZE]),
m_scratchPad(new uint8[SCRATCHSIZE]),
m_spuRam(new uint8[SPURAMSIZE]),
m_dmac(m_ram, m_intc),
m_counters(CLOCK_FREQ, m_intc),
m_thread(bind(&CPsfVm::ThreadProc, this)),
m_spuCore0(m_spuRam, SPURAMSIZE),
m_spuCore1(m_spuRam, SPURAMSIZE),
m_spu(m_spuCore0),
m_spu2(m_spuCore0, m_spuCore1),
m_spuUpdateCounter(g_spuUpdateTicks),
m_frameCounter(g_frameTicks)
{
	//Read memory map
	m_cpu.m_pMemoryMap->InsertReadMap((0 * RAMSIZE), (0 * RAMSIZE) + RAMSIZE - 1,	                m_ram,								                    0x01);
	m_cpu.m_pMemoryMap->InsertReadMap((1 * RAMSIZE), (1 * RAMSIZE) + RAMSIZE - 1,	                m_ram,								                    0x02);
	m_cpu.m_pMemoryMap->InsertReadMap((2 * RAMSIZE), (2 * RAMSIZE) + RAMSIZE - 1,	                m_ram,								                    0x03);
	m_cpu.m_pMemoryMap->InsertReadMap((3 * RAMSIZE), (3 * RAMSIZE) + RAMSIZE - 1,	                m_ram,								                    0x04);
	m_cpu.m_pMemoryMap->InsertReadMap(0x1F800000,                   0x1F8003FF,                     m_scratchPad,   									    0x05);
	m_cpu.m_pMemoryMap->InsertReadMap(HW_REG_BEGIN,					HW_REG_END,						bind(&CPsfVm::ReadIoRegister, this, _1),				0x06);

	//Write memory map
	m_cpu.m_pMemoryMap->InsertWriteMap((0 * RAMSIZE),   (0 * RAMSIZE) + RAMSIZE - 1,	m_ram,											0x01);
	m_cpu.m_pMemoryMap->InsertWriteMap((1 * RAMSIZE),   (1 * RAMSIZE) + RAMSIZE - 1,	m_ram,											0x02);
	m_cpu.m_pMemoryMap->InsertWriteMap((2 * RAMSIZE),   (2 * RAMSIZE) + RAMSIZE - 1,	m_ram,											0x03);
	m_cpu.m_pMemoryMap->InsertWriteMap((3 * RAMSIZE),   (3 * RAMSIZE) + RAMSIZE - 1,	m_ram,											0x04);
	m_cpu.m_pMemoryMap->InsertWriteMap(0x1F800000,      0x1F8003FF,                     m_scratchPad,									0x05);
	m_cpu.m_pMemoryMap->InsertWriteMap(HW_REG_BEGIN,	HW_REG_END,		                bind(&CPsfVm::WriteIoRegister, this, _1, _2),	0x06);

	//Instruction memory map
	m_cpu.m_pMemoryMap->InsertInstructionMap((0 * RAMSIZE), (0 * RAMSIZE) + RAMSIZE - 1,	m_ram,						0x01);
	m_cpu.m_pMemoryMap->InsertInstructionMap((1 * RAMSIZE), (1 * RAMSIZE) + RAMSIZE - 1,	m_ram,						0x02);
	m_cpu.m_pMemoryMap->InsertInstructionMap((2 * RAMSIZE), (2 * RAMSIZE) + RAMSIZE - 1,	m_ram,						0x03);
	m_cpu.m_pMemoryMap->InsertInstructionMap((3 * RAMSIZE), (3 * RAMSIZE) + RAMSIZE - 1,	m_ram,						0x04);

	m_cpu.m_pArch = &g_MAMIPSIV;
	m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;

#ifdef _DEBUG
	m_cpu.m_Functions.Unserialize("rawr.functions");
	m_cpu.m_Comments.Unserialize("rawr.comments");
#endif

	m_dmac.SetReceiveFunction(4, bind(&CSpuBase::ReceiveDma, &m_spuCore0, _1, _2, _3));
	m_dmac.SetReceiveFunction(8, bind(&CSpuBase::ReceiveDma, &m_spuCore1, _1, _2, _3));
}

CPsfVm::~CPsfVm()
{
#ifdef _DEBUG
	m_cpu.m_Functions.Serialize("rawr.functions");
	m_cpu.m_Comments.Serialize("rawr.comments");
#endif
	delete [] m_ram;
	delete [] m_scratchPad;
	delete [] m_spuRam;
}

void CPsfVm::Reset()
{
    if(m_bios != NULL)
    {
        delete m_bios;
		m_bios = NULL;
    }
    memset(m_ram, 0, RAMSIZE);
	memset(m_scratchPad, 0, SCRATCHSIZE);
	memset(m_spuRam, 0, SPURAMSIZE);
	m_executor.Clear();
	m_cpu.Reset();
	m_spuCore0.Reset();
	m_spuCore1.Reset();
    m_spu.Reset();
    m_spu2.Reset();
	m_counters.Reset();
	m_dmac.Reset();
	m_intc.Reset();
	m_spuHandler.Reset();
    m_frameCounter = g_frameTicks;
    m_spuUpdateCounter = g_spuUpdateTicks;
}

uint32 CPsfVm::ReadIoRegister(uint32 address)
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
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Reading an unknown hardware register (0x%0.8X).\r\n", address);
	}
	return 0;
}

uint32 CPsfVm::WriteIoRegister(uint32 address, uint32 value)
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

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
	return m_status;
}

void CPsfVm::Pause()
{
	m_mailBox.SendCall(bind(&CPsfVm::PauseImpl, this), true);
}

void CPsfVm::PauseImpl()
{
	m_status = PAUSED;
	m_OnRunningStateChange();
	m_OnMachineStateChange();
}

void CPsfVm::Resume()
{
	m_status = RUNNING;
	m_OnRunningStateChange();
}

CDebuggable CPsfVm::GetDebugInfo()
{
    CDebuggable debug;
	debug.Step = bind(&CPsfVm::Step, this);
    debug.GetCpu = bind(&CPsfVm::GetCpu, this);
    return debug;
}

CMIPS& CPsfVm::GetCpu()
{
	return m_cpu;
}

CSpuBase& CPsfVm::GetSpuCore(unsigned int coreId)
{
	if(coreId == 0)
	{
		return m_spuCore0;
	}
	else
	{
		return m_spuCore1;
	}
}

uint8* CPsfVm::GetRam()
{
    return m_ram;
}

void CPsfVm::SetBios(CBios* bios)
{
    assert(m_bios == NULL);
    m_bios = bios;
}

void CPsfVm::Step()
{
	m_singleStep = true;
	m_status = RUNNING;
	m_OnRunningStateChange();
}

unsigned int CPsfVm::ExecuteCpu(bool singleStep)
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
		int quota = singleStep ? 1 : 500;
		ticks = quota - m_executor.Execute(quota);
		assert(ticks >= 0);
        {
#pragma message("TODO: Make something better here...")
            if(m_cpu.m_State.nPC == 0x1018)
//			if(m_cpu.m_State.nPC == 0x0003D948)
			{
				ticks += (quota * 2);
            }
			else
			{
				CBasicBlock* nextBlock = m_executor.FindBlockAt(m_cpu.m_State.nPC);
				if(nextBlock != NULL && nextBlock->GetSelfLoopCount() > 5000)
				{
					//Go a little bit faster if we're "stuck"
					ticks += (quota * 2);
				}
			}
        }
		if(ticks > 0)
		{
			m_counters.Update(ticks);
		}
	}
	if(m_cpu.m_State.nHasException)
	{
		m_bios->HandleException();
	}
	return ticks;
}

void CPsfVm::ThreadProc()
{
#ifdef DEBUGGER_INCLUDED
    uint64 frameTime = (CHighResTimer::MICROSECOND / FRAMES_PER_SEC);
#endif
	while(1)
	{
		while(m_mailBox.IsPending())
		{
			m_mailBox.ReceiveCall();
		}
		if(m_status == PAUSED)
		{
            //Sleep during 100ms
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100 * 1000000;
			thread::sleep(xt);
		}
		else
		{
#ifdef DEBUGGER_INCLUDED
			int ticks = ExecuteCpu(m_singleStep);

			static int frameCounter = frameTicks;
			static uint64 currentTime = CHighResTimer::GetTime();

			frameCounter -= ticks;
			if(frameCounter <= 0)
			{
				m_intc.AssertLine(CIntc::LINE_VBLANK);
				OnNewFrame();
				frameCounter += frameTicks;
				uint64 elapsed = CHighResTimer::GetDiff(currentTime, CHighResTimer::MICROSECOND);
				int64 delay = frameTime - elapsed;
				if(delay > 0)
				{
					xtime xt;
					xtime_get(&xt, boost::TIME_UTC);
					xt.nsec += delay * 1000;
					thread::sleep(xt);
				}
				currentTime = CHighResTimer::GetTime();
			}

//			m_spuHandler.Update(m_spu);
			if(m_executor.MustBreak() || m_singleStep)
			{
				m_status = PAUSED;
				m_singleStep = false;
				m_OnMachineStateChange();
				m_OnRunningStateChange();
			}
#else
			if(m_spuHandler.HasFreeBuffers())
			{
				while(m_spuHandler.HasFreeBuffers() && !m_mailBox.IsPending())
				{
					while(m_spuUpdateCounter > 0)
					{
						int ticks = ExecuteCpu(false);
						m_spuUpdateCounter -= ticks;
						m_frameCounter -= ticks;
						if(m_frameCounter < 0)
						{
							m_frameCounter += g_frameTicks;
							m_intc.AssertLine(CIntc::LINE_VBLANK);
							OnNewFrame();
						}
					}

					m_spuHandler.Update(m_spuCore0);
					m_spuUpdateCounter += g_spuUpdateTicks;
				}
			}
			else
			{
				//Sleep during 16ms
				xtime xt;
				xtime_get(&xt, boost::TIME_UTC);
				xt.nsec += 10 * 1000000;
				thread::sleep(xt);
//				thread::yield();
			}
#endif
		}
	}
}
