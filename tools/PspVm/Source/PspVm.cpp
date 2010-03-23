#include "PspVm.h"
#include <stdexcept>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "ElfFile.h"
#include "StdStream.h"
#include "Xml/Node.h"
#include "Xml/Parser.h"
#include "Xml/Writer.h"

#define RAMSIZE				(0x02000000)

CPspVm::CPspVm() 
: m_ram(new uint8[RAMSIZE])
, m_cpu(MEMORYMAP_ENDIAN_LSBF, 0, RAMSIZE)
, m_copScu(MIPS_REGSIZE_32)
, m_copFpu(MIPS_REGSIZE_32)
, m_bios(m_cpu, m_ram, RAMSIZE)
, m_executor(m_cpu)
, m_status(PAUSED)
, m_singleStep(false)
{
	//Read memory map
	m_cpu.m_pMemoryMap->InsertReadMap(	0x00000000, RAMSIZE,		m_ram,	0x01);

	//Write memory map
	m_cpu.m_pMemoryMap->InsertWriteMap(	0x00000000, RAMSIZE,		m_ram,	0x01);

	//Instruction memory map
	m_cpu.m_pMemoryMap->InsertInstructionMap(0x00000000, RAMSIZE,	m_ram,	0x01);

	m_cpu.m_pArch			= &m_cpuArch;
	m_cpu.m_pCOP[0]			= &m_copScu;
	m_cpu.m_pCOP[1]			= &m_copFpu;
	m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;

	Reset();

    m_isThreadOver = false;
	m_thread = new boost::thread(std::tr1::bind(&CPspVm::ThreadProc, this));
}

CPspVm::~CPspVm()
{
    if(m_thread)
    {
        Pause();
        m_isThreadOver = true;
        m_thread->join();

        delete m_thread;
        m_thread = NULL;
    }

	delete [] m_ram;
}

void CPspVm::LoadModule(const char* path)
{
	assert(m_status == PAUSED);
	m_bios.LoadModule(path);
}

CDebuggable CPspVm::GetDebugInfo()
{
    CDebuggable debug;
	debug.Step = std::tr1::bind(&CPspVm::Step, this);
	debug.GetCpu = std::tr1::bind(&CPspVm::GetCpu, this);
//#ifdef DEBUGGER_INCLUDED
	debug.GetModules = std::tr1::bind(&Psp::CBios::GetModuleList, &m_bios);
//#endif
	return debug;
}

CPspVm::STATUS CPspVm::GetStatus() const
{
	return m_status;
}

void CPspVm::Reset()
{
	memset(m_ram, 0, RAMSIZE);

	m_executor.Clear();
	m_bios.Reset();
}

void CPspVm::Pause()
{
	if(m_status == PAUSED) return;
	m_mailBox.SendCall(std::tr1::bind(&CPspVm::PauseImpl, this), true);
}

void CPspVm::PauseImpl()
{
	m_status = PAUSED;
	if(!m_OnRunningStateChange.empty())
	{
		m_OnRunningStateChange();
	}
	if(!m_OnMachineStateChange.empty())
	{
		m_OnMachineStateChange();
	}
}

void CPspVm::Resume()
{
	m_status = RUNNING;
	if(!m_OnRunningStateChange.empty())
	{
		m_OnRunningStateChange();
	}
}

void CPspVm::Step()
{
	m_singleStep = true;
	m_status = RUNNING;
	m_OnRunningStateChange();
}

CMIPS& CPspVm::GetCpu()
{
	return m_cpu;
}

Psp::CBios& CPspVm::GetBios()
{
	return m_bios;
}

void CPspVm::ThreadProc()
{
	while(!m_isThreadOver)
	{
		if(m_status == PAUSED)
		{
            //Sleep during 100ms
            boost::this_thread::sleep(boost::posix_time::milliseconds(16));
		}
		else
		{
			while(m_mailBox.IsPending())
			{
				m_mailBox.ReceiveCall();
			}

			{
				int ticks = m_executor.Execute(m_singleStep ? 1 : 100);
				if(m_cpu.m_State.nHasException)
				{
					m_bios.HandleException();
				}
			}

			if(m_executor.MustBreak() || m_singleStep)
			{
				m_status = PAUSED;
				m_singleStep = false;
				m_OnMachineStateChange();
				m_OnRunningStateChange();
			}
		}
	}
}

#define TAGS_PATH				("./tags/")
#define TAGS_SECTION_TAGS		("tags")
#define TAGS_SECTION_FUNCTIONS	("functions")
#define TAGS_SECTION_COMMENTS	("comments")

std::string CPspVm::MakeTagPackagePath(const char* packageName)
{
	boost::filesystem::path tagsPath(TAGS_PATH);
	if(!boost::filesystem::exists(tagsPath))
	{
		boost::filesystem::create_directory(tagsPath);
	}
	return std::string(TAGS_PATH) + std::string(packageName) + ".tags.xml";
}

void CPspVm::LoadDebugTags(const char* packageName)
{
	try
	{
		std::string packagePath = MakeTagPackagePath(packageName);
		boost::scoped_ptr<Framework::Xml::CNode> document(Framework::Xml::CParser::ParseDocument(&Framework::CStdStream(packagePath.c_str(), "rb")));
		Framework::Xml::CNode* tagsSection = document->Select(TAGS_SECTION_TAGS);
		if(tagsSection == NULL) return;
		m_cpu.m_Functions.Unserialize(tagsSection, TAGS_SECTION_FUNCTIONS);
		m_cpu.m_Comments.Unserialize(tagsSection, TAGS_SECTION_COMMENTS);
	}
	catch(...)
	{

	}
}

void CPspVm::SaveDebugTags(const char* packageName)
{
	std::string packagePath = MakeTagPackagePath(packageName);
	boost::scoped_ptr<Framework::Xml::CNode> document(new Framework::Xml::CNode(TAGS_SECTION_TAGS, true));
	m_cpu.m_Functions.Serialize(document.get(), TAGS_SECTION_FUNCTIONS);
	m_cpu.m_Comments.Serialize(document.get(), TAGS_SECTION_COMMENTS);
	Framework::Xml::CWriter::WriteDocument(&Framework::CStdStream(packagePath.c_str(), "wb"), document.get());
}
