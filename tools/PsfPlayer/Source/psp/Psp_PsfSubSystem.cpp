#include "Psp_PsfSubSystem.h"

using namespace Psp;

#define SAMPLES_PER_FRAME	(44100 / 60)

CPsfSubSystem::CPsfSubSystem(uint32 ramSize)
: m_ram(new uint8[ramSize])
, m_ramSize(ramSize)
, m_spuRam(new uint8[SPURAMSIZE])
, m_cpu(MEMORYMAP_ENDIAN_LSBF)
, m_copScu(MIPS_REGSIZE_32)
, m_copFpu(MIPS_REGSIZE_32)
, m_executor(m_cpu, ramSize)
, m_spuCore0(m_spuRam, SPURAMSIZE, 0)
, m_spuCore1(m_spuRam, SPURAMSIZE, 1)
, m_bios(m_cpu, m_ram, ramSize)
{
	//Read memory map
	m_cpu.m_pMemoryMap->InsertReadMap(			0x00000000, m_ramSize,		m_ram,	0x01);

	//Write memory map
	m_cpu.m_pMemoryMap->InsertWriteMap(			0x00000000, m_ramSize,		m_ram,	0x01);

	//Instruction memory map
	m_cpu.m_pMemoryMap->InsertInstructionMap(	0x00000000, m_ramSize,		m_ram,	0x01);

	m_cpu.m_pArch			= &m_cpuArch;
	m_cpu.m_pCOP[0]			= &m_copScu;
	m_cpu.m_pCOP[1]			= &m_copFpu;
	m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;

	Reset();
}

CPsfSubSystem::~CPsfSubSystem()
{
	delete [] m_ram;
	delete [] m_spuRam;
}

void CPsfSubSystem::Reset()
{
	memset(m_ram, 0, m_ramSize);
	memset(m_spuRam, 0, SPURAMSIZE);
	m_executor.Reset();
	m_bios.Reset();
	m_audioStream.Truncate();

	m_bios.GetSasCore()->SetSpuInfo(&m_spuCore0, &m_spuCore1, m_spuRam, SPURAMSIZE);
	m_bios.GetAudio()->SetStream(&m_audioStream);

	m_samplesToFrame = SAMPLES_PER_FRAME;
}

CMIPS& CPsfSubSystem::GetCpu()
{
	return m_cpu;
}

uint8* CPsfSubSystem::GetRam()
{
	return m_ram;
}

CPsfBios& CPsfSubSystem::GetBios()
{
	return m_bios;
}

Iop::CSpuBase& CPsfSubSystem::GetSpuCore(unsigned int coreId)
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

int CPsfSubSystem::ExecuteCpu(bool singleStep)
{
	int ticks = m_executor.Execute(singleStep ? 1 : 100);
	if(m_cpu.m_State.nHasException)
	{
		m_bios.HandleException();
	}
	return ticks;
}

void CPsfSubSystem::Update(bool singleStep, CSoundHandler* soundHandler)
{
#ifdef DEBUGGER_INCLUDED
	ExecuteCpu(singleStep);
#else
	if(soundHandler && !soundHandler->HasFreeBuffers())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		soundHandler->RecycleBuffers();
	}
	else
	{
		const int bufferSamples = 880;
		const int bufferSize = bufferSamples * 2;
		int ticks = ExecuteCpu(false);
		if(m_audioStream.GetLength() >= bufferSize)
		{
			int16 buffer[bufferSamples];
			m_audioStream.Seek(0, Framework::STREAM_SEEK_SET);
			m_audioStream.Read(buffer, bufferSize);
			m_audioStream.Truncate();
			m_audioStream.Seek(0, Framework::STREAM_SEEK_END);

			if(soundHandler)
			{
				soundHandler->Write(buffer, bufferSamples, 44100);
			}

			m_samplesToFrame -= (bufferSamples / 2);
			if(m_samplesToFrame <= 0)
			{
				m_samplesToFrame += SAMPLES_PER_FRAME;
				if(!OnNewFrame.empty())
				{
					OnNewFrame();
				}
			}
		}
	}
#endif
}

#ifdef DEBUGGER_INCLUDED

bool CPsfSubSystem::MustBreak()
{
	return m_executor.MustBreak();
}

void CPsfSubSystem::DisableBreakpointsOnce()
{
	m_executor.DisableBreakpointsOnce();
}

CBiosDebugInfoProvider* CPsfSubSystem::GetBiosDebugInfoProvider()
{
	return &m_bios;
}

void CPsfSubSystem::LoadDebugTags(Framework::Xml::CNode* tagsNode)
{
	m_bios.LoadDebugTags(tagsNode);
}

void CPsfSubSystem::SaveDebugTags(Framework::Xml::CNode* tagsNode)
{
	m_bios.SaveDebugTags(tagsNode);
}

#endif
