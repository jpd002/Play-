#include <stdexcept>
#include <assert.h>
#include "PsxVm.h"
#include "Log.h"
#include "MA_MIPSIV.h"
#include "HighResTimer.h"

#define LOG_NAME ("psxvm")

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;
using namespace Psx;

#define CLOCK_FREQ		(44100 * 256 * 3)		//~33.8MHz
#define FRAMES_PER_SEC	(60)

CPsxVm::CPsxVm() :
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0, 0x1FFFFFFF),
m_executor(m_cpu),
m_bios(m_cpu, m_ram),
m_status(PAUSED),
m_singleStep(false),
m_ram(new uint8[RAMSIZE]),
m_scratchPad(new uint8[SCRATCHSIZE]),
m_dmac(m_ram, m_intc),
//m_counters(CLOCK_FREQ, m_intc),
m_thread(bind(&CPsxVm::ThreadProc, this))
{
	//Read memory map
	m_cpu.m_pMemoryMap->InsertReadMap((0 * RAMSIZE), (0 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x01);
	m_cpu.m_pMemoryMap->InsertReadMap((1 * RAMSIZE), (1 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x02);
	m_cpu.m_pMemoryMap->InsertReadMap((2 * RAMSIZE), (2 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x03);
	m_cpu.m_pMemoryMap->InsertReadMap((3 * RAMSIZE), (3 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x04);
	m_cpu.m_pMemoryMap->InsertReadMap(0x1F800000, 0x1F8003FF, m_scratchPad,												0x05);
	m_cpu.m_pMemoryMap->InsertReadMap(HW_REG_BEGIN,	HW_REG_END,		bind(&CPsxVm::ReadIoRegister, this, _1),			0x06);

	//Write memory map
	m_cpu.m_pMemoryMap->InsertWriteMap((0 * RAMSIZE), (0 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x01);
	m_cpu.m_pMemoryMap->InsertWriteMap((1 * RAMSIZE), (1 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x02);
	m_cpu.m_pMemoryMap->InsertWriteMap((2 * RAMSIZE), (2 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x03);
	m_cpu.m_pMemoryMap->InsertWriteMap((3 * RAMSIZE), (3 * RAMSIZE) + RAMSIZE - 1,	m_ram,								0x04);
	m_cpu.m_pMemoryMap->InsertWriteMap(0x1F800000, 0x1F8003FF, m_scratchPad,											0x05);
	m_cpu.m_pMemoryMap->InsertWriteMap(HW_REG_BEGIN,	HW_REG_END,		bind(&CPsxVm::WriteIoRegister, this, _1, _2),	0x06);

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

	m_dmac.SetReceiveFunction(4, bind(&CSpu::ReceiveDma, &m_spu, _1, _2, _3));
}

CPsxVm::~CPsxVm()
{
#ifdef _DEBUG
	m_cpu.m_Functions.Serialize("rawr.functions");
	m_cpu.m_Comments.Serialize("rawr.comments");
#endif
	delete [] m_ram;
	delete [] m_scratchPad;
}

void CPsxVm::Reset()
{
	memset(m_ram, 0, RAMSIZE);
	memset(m_scratchPad, 0, SCRATCHSIZE);
	m_executor.Clear();
	m_cpu.Reset();
	m_bios.Reset();
	m_spu.Reset();
//	m_counters.Reset();
	m_dmac.Reset();
	m_intc.Reset();
	m_spuHandler.Reset();
}

uint32 CPsxVm::ReadIoRegister(uint32 address)
{
	if(address >= CSpu::SPU_BEGIN && address <= CSpu::SPU_END)
	{
		return m_spu.ReadRegister(address);
	}
	else if(address == 0x1F801814)
	{
		return 0x14802000;
	}
	else if(address >= CDmac::ADDR_BEGIN && address <= CDmac::ADDR_END)
	{
		return m_dmac.ReadRegister(address);
	}
	else if(address >= CIntc::ADDR_BEGIN && address <= CIntc::ADDR_END)
	{
		return m_intc.ReadRegister(address);
	}
//	else if(address >= CRootCounters::ADDR_BEGIN && address <= CRootCounters::ADDR_END)
//	{
//		return m_counters.ReadRegister(address);
//	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Reading an unknown hardware register (0x%0.8X).\r\n", address);
	}
	return 0;
}

uint32 CPsxVm::WriteIoRegister(uint32 address, uint32 value)
{
	if(address >= CSpu::SPU_BEGIN && address <= CSpu::SPU_END)
	{
		m_spu.WriteRegister(address, static_cast<uint16>(value));
	}
	else if(address >= CDmac::ADDR_BEGIN && address <= CDmac::ADDR_END)
	{
		m_dmac.WriteRegister(address, value);
	}
	else if(address >= CIntc::ADDR_BEGIN && address <= CIntc::ADDR_END)
	{
		m_intc.WriteRegister(address, value);
	}
//	else if(address >= CRootCounters::ADDR_BEGIN && address <= CRootCounters::ADDR_END)
//	{
//		m_counters.WriteRegister(address, value);
//	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Writing to an unknown hardware register (0x%0.8X, 0x%0.8X).\r\n", address, value);
	}
	return 0;
}

void CPsxVm::LoadExe(uint8* exe)
{
	EXEHEADER* exeHeader(reinterpret_cast<EXEHEADER*>(exe));
	if(strncmp(reinterpret_cast<char*>(exeHeader->id), "PS-X EXE", 8))
	{
		throw runtime_error("Invalid PSX executable.");
	}

	m_cpu.m_State.nPC					= exeHeader->pc0 & 0x1FFFFFFF;
	m_cpu.m_State.nGPR[CMIPS::GP].nD0	= exeHeader->gp0;
	m_cpu.m_State.nGPR[CMIPS::SP].nD0	= exeHeader->stackAddr;

	exe += 0x800;
	if(exeHeader->textAddr != 0)
	{
		uint32 realAddr = exeHeader->textAddr & 0x1FFFFFFF;
		assert(realAddr + exeHeader->textSize <= RAMSIZE);
		memcpy(m_ram + realAddr, exe, exeHeader->textSize);
		exe += exeHeader->textSize;
	}
}

CVirtualMachine::STATUS CPsxVm::GetStatus() const
{
	return m_status;
}

void CPsxVm::Pause()
{
	m_mailBox.SendCall(bind(&CPsxVm::PauseImpl, this), true);
}

void CPsxVm::PauseImpl()
{
	m_status = PAUSED;
	m_OnRunningStateChange();
	m_OnMachineStateChange();
}

void CPsxVm::Resume()
{
	m_status = RUNNING;
	m_OnRunningStateChange();
}

CMIPS& CPsxVm::GetCpu()
{
	return m_cpu;
}

CSpu& CPsxVm::GetSpu()
{
	return m_spu;
}

void CPsxVm::Step()
{
	m_singleStep = true;
	m_status = RUNNING;
	m_OnRunningStateChange();
}

unsigned int CPsxVm::ExecuteCpu(bool singleStep)
{
	int ticks = 0;
    if(!m_cpu.m_State.nHasException)
    {
		if(m_intc.IsInterruptPending())
		{
			m_bios.HandleInterrupt();
        }
    }
	if(!m_cpu.m_State.nHasException)
	{
		int quota = singleStep ? 1 : 500;
		ticks = quota - m_executor.Execute(quota);
		assert(ticks >= 0);
        {
            CBasicBlock* nextBlock = m_executor.FindBlockAt(m_cpu.m_State.nPC);
            if(nextBlock != NULL && nextBlock->GetSelfLoopCount() > 5000)
            {
				//Go a little bit faster if we're "stuck"
				ticks += (quota * 2);
            }
        }
		if(ticks > 0)
		{
//			m_counters.Update(ticks);
		}
	}
	if(m_cpu.m_State.nHasException)
	{
		m_bios.HandleException();
	}
	return ticks;
}

void CPsxVm::ThreadProc()
{
	const int frameTicks = (CLOCK_FREQ / FRAMES_PER_SEC);
	const int spuUpdateTicks = (4 * CLOCK_FREQ / 1000);
	uint64 frameTime = (CHighResTimer::MICROSECOND / FRAMES_PER_SEC);
	int frameCounter = frameTicks;
	int spuUpdateCounter = spuUpdateTicks;
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
					while(spuUpdateCounter > 0)
					{
						int ticks = ExecuteCpu(false);
						spuUpdateCounter -= ticks;
						frameCounter -= ticks;
						if(frameCounter < 0)
						{
							frameCounter += frameTicks;
							m_intc.AssertLine(CIntc::LINE_VBLANK);
							OnNewFrame();
						}
					}

					m_spuHandler.Update(m_spu);
					spuUpdateCounter += spuUpdateTicks;
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
