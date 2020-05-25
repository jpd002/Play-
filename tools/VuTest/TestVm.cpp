#include "TestVm.h"
#include "Ps2Const.h"

CTestVm::CTestVm()
    : m_cpu(MEMORYMAP_ENDIAN_LSBF)
    , m_executor(m_cpu, PS2::MICROMEM1SIZE)
    , m_vuMem(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::VUMEM1SIZE, 0x10)))
    , m_microMem(reinterpret_cast<uint8*>(framework_aligned_alloc(PS2::MICROMEM1SIZE, 0x10)))
    , m_maVu(PS2::VUMEM1SIZE - 1)
{
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, 0x00003FFF, m_vuMem, 0x00);
	m_cpu.m_pMemoryMap->InsertWriteMap(0x00000000, 0x00003FFF, m_vuMem, 0x00);

	m_cpu.m_pMemoryMap->InsertInstructionMap(0x00000000, 0x00003FFF, m_microMem, 0x01);

	m_cpu.m_pArch = &m_maVu;
	m_cpu.m_pAddrTranslator = CMIPS::TranslateAddress64;

	m_cpu.m_vuMem = m_vuMem;
}

CTestVm::~CTestVm()
{
	framework_aligned_free(m_vuMem);
	framework_aligned_free(m_microMem);
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
	assert(m_cpu.m_State.nHasException == 0);
	while(!m_cpu.m_State.nHasException)
	{
		m_executor.Execute(100);
	}
	m_cpu.m_State.nHasException = 0;
}
