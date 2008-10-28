#include "PsfVm.h"
#include "Ps2Const.h"
#include "MA_MIPSIV.h"

using namespace PS2;
using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;
using namespace Iop;

#define PSF_DEVICENAME "psf"

CPsfVm::CPsfVm() :
m_ram(new uint8[IOPRAMSIZE]),
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0x00000000, IOPRAMSIZE),
m_dmac(m_ram, m_intc),
m_executor(m_cpu),
m_status(PAUSED),
m_singleStep(false),
m_spu(CSpu2::REGS_BEGIN),
m_bios(0x1000, m_cpu, m_ram, IOPRAMSIZE, *reinterpret_cast<CSIF*>(NULL), *reinterpret_cast<CISO9660**>(NULL)),
m_thread(bind(&CPsfVm::ThreadProc, this))
{
    //IOP context setup
    {
        //Read map
	    m_cpu.m_pMemoryMap->InsertReadMap(0x00000000,					IOPRAMSIZE - 1,				m_ram,											0x00);
		m_cpu.m_pMemoryMap->InsertReadMap(CIntc::ADDR_BEGIN,			CIntc::ADDR_END,			bind(&CIntc::ReadRegister, &m_intc, _1),		0x01);
		m_cpu.m_pMemoryMap->InsertReadMap(CDmac::DMAC_ZONE1_START,		CDmac::DMAC_ZONE1_END,		bind(&CDmac::ReadRegister, &m_dmac, _1),        0x02);
        m_cpu.m_pMemoryMap->InsertReadMap(CSpu2::REGS_BEGIN,			CSpu2::REGS_END,			bind(&CSpu2::ReadRegister, &m_spu, _1),         0x03);

        //Write map
        m_cpu.m_pMemoryMap->InsertWriteMap(0x00000000,                  IOPRAMSIZE - 1,	            m_ram,											0x00);
		m_cpu.m_pMemoryMap->InsertWriteMap(CIntc::ADDR_BEGIN,			CIntc::ADDR_END,			bind(&CIntc::WriteRegister, &m_intc, _1, _2),	0x01);
        m_cpu.m_pMemoryMap->InsertWriteMap(CDmac::DMAC_ZONE1_START,     CDmac::DMAC_ZONE1_END,      bind(&CDmac::WriteRegister, &m_dmac, _1, _2),   0x02);
        m_cpu.m_pMemoryMap->InsertWriteMap(CSpu2::REGS_BEGIN,           CSpu2::REGS_END,	    	bind(&CSpu2::WriteRegister, &m_spu, _1, _2),    0x03);

	    //Instruction map
        m_cpu.m_pMemoryMap->InsertInstructionMap(0x00000000, IOPRAMSIZE - 1, m_ram,  0x00);

	    m_cpu.m_pArch			= &g_MAMIPSIV;

		m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;
    }

    m_dmac.SetReceiveFunction(4, bind(&Spu2::CCore::ReceiveDma, m_spu.GetCore(0), _1, _2, _3));
}

CPsfVm::~CPsfVm()
{

}

CDebuggable CPsfVm::GetIopDebug()
{
    CDebuggable debug;
	debug.Step = bind(&CPsfVm::Step, this);
    debug.GetCpu = bind(&CPsfVm::GetCpu, this);
    return debug;
}

void CPsfVm::LoadPsf(const CPsfDevice::PsfFile& psfFile)
{
    Iop::CIoman* ioman = m_bios.GetIoman();
    ioman->RegisterDevice(PSF_DEVICENAME, new CPsfDevice(psfFile));

    string execPath = string(PSF_DEVICENAME) + ":/psf2.irx";
    m_bios.LoadAndStartModule(execPath.c_str(), NULL, 0);
}

void CPsfVm::Reset()
{
    memset(m_ram, 0, IOPRAMSIZE);
	m_intc.Reset();
	m_bios.Reset();
    m_dmac.Reset();
}

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
    return m_status;
}

void CPsfVm::Resume()
{
	m_status = RUNNING;
}

void CPsfVm::Pause()
{
	m_status = PAUSED;
}

void CPsfVm::Step()
{
	m_singleStep = true;
	m_status = RUNNING;
	m_OnRunningStateChange();
}

CMIPS& CPsfVm::GetCpu()
{
    return m_cpu;
}

unsigned int CPsfVm::ExecuteCpu(bool singleStep)
{
	int ticks = 0;
    if(!m_cpu.m_State.nHasException)
    {
		if(m_intc.HasPendingInterrupt())
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

void CPsfVm::ThreadProc()
{
	const int frameTicks = 20000;
//	const int frameTicks = (CLOCK_FREQ / FRAMES_PER_SEC);
//	const int spuUpdateTicks = (4 * CLOCK_FREQ / 1000);
//	uint64 frameTime = (CHighResTimer::MICROSECOND / FRAMES_PER_SEC);
//	int frameCounter = frameTicks;
//	int spuUpdateCounter = spuUpdateTicks;
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

			frameCounter -= ticks;
			if(frameCounter <= 0)
			{
//				m_intc.AssertLine(CIntc::LINE_VBLANK);
				OnNewFrame();
				frameCounter += frameTicks;
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
/*
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
*/
#endif
		}
	}
}
