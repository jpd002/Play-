#include "TestVm.h"

CTestVm::CTestVm()
: m_cpu(MEMORYMAP_ENDIAN_LSBF)
, m_executor(m_cpu, (RAM_SIZE))
, m_ram(new uint8[RAM_SIZE])
{
	m_cpu.m_pMemoryMap->InsertReadMap(0, RAM_SIZE - 1, m_ram, 0x01);
	m_cpu.m_pMemoryMap->InsertWriteMap(0, RAM_SIZE - 1, m_ram, 0x01);
	m_cpu.m_pMemoryMap->InsertInstructionMap(0, RAM_SIZE - 1, m_ram, 0x01);

	m_cpu.m_pArch = &m_cpuArch;
	m_cpu.m_pAddrTranslator = &CMIPS::TranslateAddress64;
}

CTestVm::~CTestVm()
{

}

void CTestVm::Reset()
{
	memset(m_ram, 0, RAM_SIZE);
	m_cpu.Reset();
	m_executor.Reset();
}

void CTestVm::ExecuteTest(uint32 startAddress)
{
	m_cpu.m_State.nPC = startAddress;
	while(!m_cpu.m_State.nHasException)
	{
		m_executor.Execute(100);
	}
}
