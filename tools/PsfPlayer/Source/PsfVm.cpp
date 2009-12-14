#include <stdexcept>
#include <assert.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "PsfVm.h"
#include "Log.h"
#include "MA_MIPSIV.h"
#include "HighResTimer.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "Ps2Const.h"

#define LOG_NAME ("psfvm")

using namespace std;
using namespace std::tr1;
using namespace Iop;
using namespace Framework;
namespace filesystem = boost::filesystem;

#define FRAMES_PER_SEC	(60)

const int g_frameTicks = (PS2::IOP_CLOCK_FREQ / FRAMES_PER_SEC);
const int g_spuUpdateTicks = (1 * PS2::IOP_CLOCK_FREQ / 1000);

CPsfVm::CPsfVm() :
m_status(PAUSED),
m_singleStep(false),
m_spuUpdateCounter(g_spuUpdateTicks),
m_spuHandler(NULL),
m_frameCounter(g_frameTicks)
{
    m_isThreadOver = false;
    m_thread = new boost::thread(bind(&CPsfVm::ThreadProc, this));
}

CPsfVm::~CPsfVm()
{
    if(m_thread)
    {
        Pause();
        m_isThreadOver = true;
        m_thread->join();

        delete m_thread;
        m_thread = NULL;

        assert(m_spuHandler == NULL);
    }
}

void CPsfVm::Reset()
{
	m_iop.Reset();
	if(m_spuHandler)
	{
		m_spuHandler->Reset();
	}
    m_frameCounter = g_frameTicks;
    m_spuUpdateCounter = g_spuUpdateTicks;
}

#ifdef DEBUGGER_INCLUDED

#define TAGS_PATH				("./tags/")
#define TAGS_SECTION_TAGS		("tags")
#define TAGS_SECTION_FUNCTIONS	("functions")
#define TAGS_SECTION_COMMENTS	("comments")

string CPsfVm::MakeTagPackagePath(const char* packageName)
{
	filesystem::path tagsPath(TAGS_PATH);
	if(!filesystem::exists(tagsPath))
	{
		filesystem::create_directory(tagsPath);
	}
	return string(TAGS_PATH) + string(packageName) + ".tags.xml";
}

void CPsfVm::LoadDebugTags(const char* packageName)
{
	try
	{
		string packagePath = MakeTagPackagePath(packageName);
		boost::scoped_ptr<Xml::CNode> document(Xml::CParser::ParseDocument(&CStdStream(packagePath.c_str(), "rb")));
		Xml::CNode* tagsSection = document->Select(TAGS_SECTION_TAGS);
		if(tagsSection == NULL) return;
		m_iop.m_cpu.m_Functions.Unserialize(tagsSection, TAGS_SECTION_FUNCTIONS);
		m_iop.m_cpu.m_Comments.Unserialize(tagsSection, TAGS_SECTION_COMMENTS);
		m_iop.m_bios->LoadDebugTags(tagsSection);
	}
	catch(...)
	{

	}
}

void CPsfVm::SaveDebugTags(const char* packageName)
{
	string packagePath = MakeTagPackagePath(packageName);
	boost::scoped_ptr<Xml::CNode> document(new Xml::CNode(TAGS_SECTION_TAGS, true));
	m_iop.m_cpu.m_Functions.Serialize(document.get(), TAGS_SECTION_FUNCTIONS);
	m_iop.m_cpu.m_Comments.Serialize(document.get(), TAGS_SECTION_COMMENTS);
	m_iop.m_bios->SaveDebugTags(document.get());
	Xml::CWriter::WriteDocument(&CStdStream(packagePath.c_str(), "wb"), document.get());
}

#endif

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
	return m_status;
}

void CPsfVm::Pause()
{
	if(m_status == PAUSED) return;
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
	if(!m_OnRunningStateChange.empty())
	{
		m_OnRunningStateChange();		
	}
}

CDebuggable CPsfVm::GetDebugInfo()
{
    CDebuggable debug;
	debug.Step = bind(&CPsfVm::Step, this);
    debug.GetCpu = bind(&CPsfVm::GetCpu, this);
#ifdef DEBUGGER_INCLUDED
	debug.GetModules = bind(&Iop::CBiosBase::GetModuleList, m_iop.m_bios);
#endif
	return debug;
}

CMIPS& CPsfVm::GetCpu()
{
	return m_iop.m_cpu;
}

CSpuBase& CPsfVm::GetSpuCore(unsigned int coreId)
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

uint8* CPsfVm::GetRam()
{
    return m_iop.m_ram;
}

void CPsfVm::SetBios(const Iop::CSubSystem::BiosPtr& bios)
{
	m_iop.SetBios(bios);
}

void CPsfVm::Step()
{
	m_singleStep = true;
	m_status = RUNNING;
	m_OnRunningStateChange();
}

void CPsfVm::SetSpuHandler(const SpuHandlerFactory& factory)
{
	m_mailBox.SendCall(bind(&CPsfVm::SetSpuHandlerImpl, this, factory));
}

void CPsfVm::SetSpuHandlerImpl(const SpuHandlerFactory& factory)
{
	if(m_spuHandler)
	{
		delete m_spuHandler;
		m_spuHandler = NULL;
	}
	m_spuHandler = factory();
}

void CPsfVm::SetReverbEnabled(bool enabled)
{
	m_mailBox.SendCall(bind(&CPsfVm::SetReverbEnabledImpl, this, enabled));
}

void CPsfVm::SetReverbEnabledImpl(bool enabled)
{
	m_iop.m_spuCore0.SetReverbEnabled(enabled);
	m_iop.m_spuCore1.SetReverbEnabled(enabled);
}

void CPsfVm::SetVolumeAdjust(float volumeAdjust)
{
	m_mailBox.SendCall(bind(&CPsfVm::SetVolumeAdjustImpl, this, volumeAdjust));
}

void CPsfVm::SetVolumeAdjustImpl(float volumeAdjust)
{
	m_iop.m_spuCore0.SetVolumeAdjust(volumeAdjust);
	m_iop.m_spuCore1.SetVolumeAdjust(volumeAdjust);
}

void CPsfVm::ThreadProc()
{
#ifdef DEBUGGER_INCLUDED
    uint64 frameTime = (CHighResTimer::MICROSECOND / FRAMES_PER_SEC);
#endif

//    const int sampleCount = 176;
    const int sampleCount = 44;
    const int blockSize = sampleCount * 2;
    const int blockCount = 10;
    int16 samples[blockSize * blockCount];
    int16 samplesSpu1[blockSize];
    int currentBlock = 0;

	while(!m_isThreadOver)
	{
		while(m_mailBox.IsPending())
		{
			m_mailBox.ReceiveCall();
		}
		if(m_status == PAUSED)
		{
            //Sleep during 100ms
            boost::this_thread::sleep(boost::posix_time::milliseconds(16));
		}
		else
		{
#ifdef DEBUGGER_INCLUDED
			int ticks = m_iop.ExecuteCpu(m_singleStep);

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
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.nsec += delay * 1000;
					boost::thread::sleep(xt);
				}
				currentTime = CHighResTimer::GetTime();
			}

//			m_spuHandler.Update(m_spu);
			if(m_iop.m_executor.MustBreak() || m_singleStep)
			{
				m_status = PAUSED;
				m_singleStep = false;
				m_OnMachineStateChange();
				m_OnRunningStateChange();
			}
#else
            if(m_spuHandler && !m_spuHandler->HasFreeBuffers())
            {
                boost::this_thread::sleep(boost::posix_time::milliseconds(16));
                m_spuHandler->RecycleBuffers();
            }
            else
            {
                int ticks = m_iop.ExecuteCpu(false);
                //if(!OnBufferWrite.empty()) OnBufferWrite(ticks);
                m_spuUpdateCounter -= ticks;
                m_frameCounter -= ticks;
                if(m_spuUpdateCounter < 0)
                {
                    m_spuUpdateCounter += g_spuUpdateTicks;
                    unsigned int blockOffset = (blockSize * currentBlock);
                    int16* samplesSpu0 = samples + blockOffset;
                    
                    memset(samplesSpu0, 0, blockSize * 2);
                    m_iop.m_spuCore0.Render(samplesSpu0, blockSize, 44100);

                    memset(samplesSpu1, 0, blockSize * 2);
                    m_iop.m_spuCore1.Render(samplesSpu1, blockSize, 44100);

                    for(unsigned int i = 0; i < blockSize; i++)
                    {
	                    int32 resultSample = static_cast<int32>(samplesSpu0[i]) + static_cast<int32>(samplesSpu1[i]);
	                    resultSample = max<int32>(resultSample, SHRT_MIN);
	                    resultSample = min<int32>(resultSample, SHRT_MAX);
	                    samplesSpu0[i] = static_cast<int16>(resultSample);
                    }
                    //if(!OnBufferWrite.empty()) OnBufferWrite(blockSize / 2);
                    currentBlock++;
                    if(currentBlock == blockCount)
                    {
                        if(m_spuHandler)
                        {
                            m_spuHandler->Write(samples, blockSize * blockCount, 44100);
                        }
                        currentBlock = 0;
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
			//	//Sleep during 16ms
   //             boost::this_thread::sleep(boost::posix_time::milliseconds(10));
			//}
#endif
		}
	}

    if(m_spuHandler != NULL)
    {
        delete m_spuHandler;
        m_spuHandler = NULL;
    }
}
