#include "Psp_PsfSubSystem.h"

using namespace Psp;

CPsfSubSystem::CPsfSubSystem(uint32 ramSize)
: m_ram(new uint8[ramSize])
, m_ramSize(ramSize)
, m_spuRam(new uint8[SPURAMSIZE])
, m_cpu(MEMORYMAP_ENDIAN_LSBF, 0, ramSize)
, m_copScu(MIPS_REGSIZE_32)
, m_copFpu(MIPS_REGSIZE_32)
, m_executor(m_cpu)
, m_spuCore0(m_spuRam, SPURAMSIZE)
, m_spuCore1(m_spuRam, SPURAMSIZE)
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
	m_executor.Clear();
	m_bios.Reset();
	m_audioStream.Truncate();

	m_bios.GetSasCore()->SetSpuInfo(&m_spuCore0, &m_spuCore1, m_spuRam, SPURAMSIZE);
	m_bios.GetAudio()->SetStream(&m_audioStream);
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
	if(soundHandler && !soundHandler->HasFreeBuffers())
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(16));
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
		}
		//m_spuUpdateCounter -= ticks;
		//m_frameCounter -= ticks;
		//if(m_spuUpdateCounter < 0)
		//{
		//	m_spuUpdateCounter += g_spuUpdateTicks;
		//	unsigned int blockOffset = (BLOCK_SIZE * m_currentBlock);
		//	int16* samplesSpu0 = m_samples + blockOffset;
		//       
		//	m_iop.m_spuCore0.Render(samplesSpu0, BLOCK_SIZE, 44100);

		//	if(m_iop.m_spuCore1.IsEnabled())
		//	{
		//		int16 samplesSpu1[BLOCK_SIZE];
		//		m_iop.m_spuCore1.Render(samplesSpu1, BLOCK_SIZE, 44100);

		//		for(unsigned int i = 0; i < BLOCK_SIZE; i++)
		//		{
		//			int32 resultSample = static_cast<int32>(samplesSpu0[i]) + static_cast<int32>(samplesSpu1[i]);
		//			resultSample = std::max<int32>(resultSample, SHRT_MIN);
		//			resultSample = std::min<int32>(resultSample, SHRT_MAX);
		//			samplesSpu0[i] = static_cast<int16>(resultSample);
		//		}
		//	}

		//	m_currentBlock++;
		//	if(m_currentBlock == BLOCK_COUNT)
		//	{
		//		if(soundHandler)
		//		{
		//			soundHandler->Write(m_samples, BLOCK_SIZE * BLOCK_COUNT, 44100);
		//		}
		//		m_currentBlock = 0;
		//	}
		//}
		//if(m_frameCounter < 0)
		//{
		//	m_frameCounter += g_frameTicks;
		//	m_iop.m_intc.AssertLine(CIntc::LINE_VBLANK);
		//	if(!OnNewFrame.empty())
		//	{
		//		OnNewFrame();
		//	}
		//}
	}
}
