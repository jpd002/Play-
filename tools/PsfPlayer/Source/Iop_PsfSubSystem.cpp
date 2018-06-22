#include "Iop_PsfSubSystem.h"
#include "Ps2Const.h"
#include <thread>

using namespace Iop;

#define FRAMES_PER_SEC (60)

CPsfSubSystem::CPsfSubSystem(bool ps2Mode)
    : m_iop(ps2Mode)
    , m_frameTicks(0)
    , m_spuUpdateTicks(0)
    , m_spuUpdateCounter(0)
    , m_frameCounter(0)
    , m_currentBlock(0)
{
	uint32 cpuFreq = ps2Mode ? PS2::IOP_CLOCK_OVER_FREQ : PS2::IOP_CLOCK_BASE_FREQ;
	m_frameTicks = (cpuFreq / FRAMES_PER_SEC);
	m_spuUpdateTicks = (cpuFreq / 1000);

	Reset();
}

void CPsfSubSystem::Reset()
{
	m_iop.Reset();
	m_currentBlock = 0;
	m_frameCounter = m_frameTicks;
	m_spuUpdateCounter = m_spuUpdateTicks;
}

CMIPS& CPsfSubSystem::GetCpu()
{
	return m_iop.m_cpu;
}

CSpuBase& CPsfSubSystem::GetSpuCore(unsigned int coreId)
{
	if(coreId == 0)
	{
		return m_iop.m_spuCore0;
	}
	else
	{
		return m_iop.m_spuCore1;
	}
}

Iop::CBiosBase* CPsfSubSystem::GetBios() const
{
	return m_iop.m_bios.get();
}

void CPsfSubSystem::Update(bool singleStep, CSoundHandler* soundHandler)
{
	if(soundHandler && !soundHandler->HasFreeBuffers())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		soundHandler->RecycleBuffers();
	}
	else
	{
		//Execute CPU and step devices
		{
			int ticks = 0;
			int quota = 500;
			if(m_iop.IsCpuIdle())
			{
				ticks += (quota * 2);
				quota /= 50;
			}
			ticks += m_iop.ExecuteCpu(singleStep ? 1 : quota);
			m_iop.CountTicks(ticks);
			m_spuUpdateCounter -= ticks;
			m_frameCounter -= ticks;
		}

		if(m_spuUpdateCounter < 0)
		{
			m_spuUpdateCounter += m_spuUpdateTicks;
			unsigned int blockOffset = (BLOCK_SIZE * m_currentBlock);
			int16* samplesSpu0 = m_samples + blockOffset;

			m_iop.m_spuCore0.Render(samplesSpu0, BLOCK_SIZE, 44100);

			if(m_iop.m_spuCore1.IsEnabled())
			{
				int16 samplesSpu1[BLOCK_SIZE];
				m_iop.m_spuCore1.Render(samplesSpu1, BLOCK_SIZE, 44100);

				for(unsigned int i = 0; i < BLOCK_SIZE; i++)
				{
					int32 resultSample = static_cast<int32>(samplesSpu0[i]) + static_cast<int32>(samplesSpu1[i]);
					resultSample = std::max<int32>(resultSample, SHRT_MIN);
					resultSample = std::min<int32>(resultSample, SHRT_MAX);
					samplesSpu0[i] = static_cast<int16>(resultSample);
				}
			}

			m_currentBlock++;
			if(m_currentBlock == BLOCK_COUNT)
			{
				if(soundHandler)
				{
					soundHandler->Write(m_samples, BLOCK_SIZE * BLOCK_COUNT, 44100);
				}
				m_currentBlock = 0;
			}
		}
		if(m_frameCounter < 0)
		{
			m_frameCounter += m_frameTicks;
			m_iop.m_intc.AssertLine(CIntc::LINE_VBLANK);
			m_iop.NotifyVBlankStart();
			m_iop.NotifyVBlankEnd();
			if(!OnNewFrame.empty())
			{
				OnNewFrame();
			}
		}
	}
}

#ifdef DEBUGGER_INCLUDED

bool CPsfSubSystem::MustBreak()
{
	return m_iop.m_executor.MustBreak();
}

void CPsfSubSystem::DisableBreakpointsOnce()
{
	m_iop.m_executor.DisableBreakpointsOnce();
}

CBiosDebugInfoProvider* CPsfSubSystem::GetBiosDebugInfoProvider()
{
	return m_iop.m_bios.get();
}

void CPsfSubSystem::LoadDebugTags(Framework::Xml::CNode* tagsNode)
{
	m_iop.m_bios->LoadDebugTags(tagsNode);
}

void CPsfSubSystem::SaveDebugTags(Framework::Xml::CNode* tagsNode)
{
	m_iop.m_bios->SaveDebugTags(tagsNode);
}

#endif
