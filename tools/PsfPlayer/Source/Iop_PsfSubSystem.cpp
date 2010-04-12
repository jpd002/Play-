#include "Iop_PsfSubSystem.h"
#include "Ps2Const.h"
#include "HighResTimer.h"

using namespace Iop;

#define FRAMES_PER_SEC	(60)

const int g_frameTicks = (PS2::IOP_CLOCK_FREQ / FRAMES_PER_SEC);
const int g_spuUpdateTicks = (1 * PS2::IOP_CLOCK_FREQ / 1000);

CPsfSubSystem::CPsfSubSystem()
: m_spuUpdateCounter(g_spuUpdateTicks)
, m_frameCounter(g_frameTicks)
, m_currentBlock(0)
{
	Reset();
}

CPsfSubSystem::~CPsfSubSystem()
{

}

void CPsfSubSystem::Reset()
{
	m_iop.Reset();
	m_currentBlock = 0;
	m_frameCounter = g_frameTicks;
	m_spuUpdateCounter = g_spuUpdateTicks;
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

uint8* CPsfSubSystem::GetRam()
{
	return m_iop.m_ram;
}

void CPsfSubSystem::SetBios(const CSubSystem::BiosPtr& bios)
{
	m_iop.SetBios(bios);
}

void CPsfSubSystem::Update(bool singleStep, CSoundHandler* soundHandler)
{
#ifdef DEBUGGER_INCLUDED
    uint64 frameTime = (CHighResTimer::MICROSECOND / FRAMES_PER_SEC);
	int ticks = m_iop.ExecuteCpu(singleStep);

	static int frameCounter = g_frameTicks;
	static uint64 currentTime = CHighResTimer::GetTime();

	frameCounter -= ticks;
	if(frameCounter <= 0)
	{
		m_iop.m_intc.AssertLine(CIntc::LINE_VBLANK);
		OnNewFrame();
		frameCounter += g_frameTicks;
		uint64 elapsed = CHighResTimer::GetDiff(currentTime, CHighResTimer::MICROSECOND);
		int64 delay = frameTime - elapsed;
		if(delay > 0)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
		}
		currentTime = CHighResTimer::GetTime();
	}

	//m_spuHandler.Update(m_spu);
#else
	if(soundHandler && !soundHandler->HasFreeBuffers())
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(16));
		soundHandler->RecycleBuffers();
	}
	else
	{
		int ticks = m_iop.ExecuteCpu(false);
		m_spuUpdateCounter -= ticks;
		m_frameCounter -= ticks;
		if(m_spuUpdateCounter < 0)
		{
			m_spuUpdateCounter += g_spuUpdateTicks;
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
			m_frameCounter += g_frameTicks;
			m_iop.m_intc.AssertLine(CIntc::LINE_VBLANK);
			if(!OnNewFrame.empty())
			{
				OnNewFrame();
			}
		}
	}
	//if(m_spuHandler && m_spuHandler->HasFreeBuffers())
	//{
	//	while(m_spuHandler->HasFreeBuffers() && !m_mailBox.IsPending())
	//	{
	//		while(m_spuUpdateCounter > 0)
	//		{
	//			int ticks = m_iop.ExecuteCpu(false);
	//			m_spuUpdateCounter -= ticks;
	//			m_frameCounter -= ticks;
	//			if(m_frameCounter < 0)
	//			{
	//				m_frameCounter += g_frameTicks;
	//				m_iop.m_intc.AssertLine(CIntc::LINE_VBLANK);
	//				if(!OnNewFrame.empty())
	//				{
	//					OnNewFrame();
	//				}
	//			}
	//		}

	//		m_spuHandler->Update(m_iop.m_spuCore0, m_iop.m_spuCore1);
	//		m_spuUpdateCounter += g_spuUpdateTicks;
	//	}
	//}
	//else
	//{
	//		//Sleep during 16ms
	//      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	//}
#endif
}

#ifdef DEBUGGER_INCLUDED

bool CPsfSubSystem::MustBreak()
{
	return m_iop.m_executor.MustBreak();
}

MipsModuleList CPsfSubSystem::GetModuleList()
{
	return m_iop.m_bios->GetModuleList();
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
