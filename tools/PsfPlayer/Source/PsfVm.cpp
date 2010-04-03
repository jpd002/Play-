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

#define LOG_NAME ("psfvm")

using namespace std;
using namespace std::tr1;
using namespace Iop;
using namespace Framework;
namespace filesystem = boost::filesystem;

CPsfVm::CPsfVm() :
m_status(PAUSED),
m_singleStep(false),
m_soundHandler(NULL)
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

        assert(m_soundHandler == NULL);
    }
}

void CPsfVm::Reset()
{
	m_subSystem.reset();
	if(m_soundHandler)
	{
		m_soundHandler->Reset();
	}
}

void CPsfVm::SetSubSystem(const PsfVmSubSystemPtr& subSystem)
{
	m_subSystem = subSystem;
	m_subSystem->OnNewFrame.connect(std::tr1::cref(OnNewFrame));
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
	return m_subSystem->GetCpu();
}

CSpuBase& CPsfVm::GetSpuCore(unsigned int coreId)
{
	return m_subSystem->GetSpuCore(coreId);
}

uint8* CPsfVm::GetRam()
{
	return m_subSystem->GetRam();
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
	if(m_soundHandler)
	{
		delete m_soundHandler;
		m_soundHandler = NULL;
	}
	m_soundHandler = factory();
}

void CPsfVm::SetReverbEnabled(bool enabled)
{
	m_mailBox.SendCall(bind(&CPsfVm::SetReverbEnabledImpl, this, enabled));
}

void CPsfVm::SetReverbEnabledImpl(bool enabled)
{
	assert(m_subSystem != NULL);
	if(m_subSystem == NULL) return;
	m_subSystem->GetSpuCore(0).SetReverbEnabled(enabled);
	m_subSystem->GetSpuCore(1).SetReverbEnabled(enabled);
}

void CPsfVm::SetVolumeAdjust(float volumeAdjust)
{
	m_mailBox.SendCall(bind(&CPsfVm::SetVolumeAdjustImpl, this, volumeAdjust));
}

void CPsfVm::SetVolumeAdjustImpl(float volumeAdjust)
{
	assert(m_subSystem != NULL);
	if(m_subSystem == NULL) return;
	m_subSystem->GetSpuCore(0).SetVolumeAdjust(volumeAdjust);
	m_subSystem->GetSpuCore(1).SetVolumeAdjust(volumeAdjust);
}

void CPsfVm::ThreadProc()
{
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
			m_subSystem->Update(false, m_soundHandler);
		}
	}

    if(m_soundHandler != NULL)
    {
        delete m_soundHandler;
        m_soundHandler = NULL;
    }
}
