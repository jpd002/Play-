#include "TestVm.h"
#include "Ps2Const.h"

CTestVm::CTestVm()
: m_cpu(MEMORYMAP_ENDIAN_LSBF)
, m_executor(m_cpu)
, m_vuMem(new uint8[PS2::VUMEM1SIZE])
, m_microMem(new uint8[PS2::MICROMEM1SIZE])
, m_maVu(PS2::VUMEM1SIZE - 1)
{
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_vuMem,	0x00);
	m_cpu.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_vuMem,	0x00);

	m_cpu.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00003FFF, m_microMem, 0x01);

	m_cpu.m_pArch			= &m_maVu;
	m_cpu.m_pAddrTranslator	= CMIPS::TranslateAddress64;

	m_cpu.m_vuMem = m_vuMem;
}

CTestVm::~CTestVm()
{
	delete [] m_vuMem;
	delete [] m_microMem;
}

void CTestVm::Reset()
{
	m_cpu.Reset();
	m_executor.Reset();
	memset(m_vuMem, 0, PS2::VUMEM1SIZE);
	memset(m_microMem, 0, PS2::MICROMEM1SIZE);
}

void CTestVm::ExecuteTest(uint32 startAddress)
{
	m_cpu.m_State.nPC = startAddress;
	while(!m_cpu.m_State.nHasException)
	{
		m_executor.Execute(100);
	}
}
