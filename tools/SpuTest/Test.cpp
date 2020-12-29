#include "Test.h"

#include <vector>
#include <cassert>

#include "Ps2Const.h"

CTest::CTest()
: m_ram(new uint8[PS2::SPU_RAM_SIZE])
, m_spuCore0(m_ram, PS2::SPU_RAM_SIZE, 0)
, m_spuCore1(m_ram, PS2::SPU_RAM_SIZE, 1)
, m_spu(m_spuCore0, m_spuCore1)
{
	memset(m_ram, 0, PS2::SPU_RAM_SIZE);
}

CTest::~CTest()
{
	delete [] m_ram;
}

void CTest::RunSpu(unsigned int ticks)
{
	static const unsigned int DST_SAMPLE_RATE = 44100;
	unsigned int blockSize = ticks * 2;
	std::vector<int16> samples(blockSize);
	m_spuCore0.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
	m_spuCore1.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
}

void CTest::SetCoreRegister(unsigned int coreIndex, uint32 address, uint32 value)
{
	assert(coreIndex == 0);
	m_spu.WriteRegister(address, value);
}

void CTest::SetCoreAddress(unsigned int coreIndex, uint32 registerAddress, uint32 targetAddress)
{
	assert((registerAddress & 0x2) == 0);
	SetCoreRegister(coreIndex, registerAddress + 0, (targetAddress >> 17) & 0xFFFE);
	SetCoreRegister(coreIndex, registerAddress + 2, (targetAddress >> 1) & 0xFFFE);
}

void CTest::SetVoiceRegister(unsigned int coreIndex, uint32 voiceIndex, uint32 address, uint32 value)
{
	uint32 voiceAddress = address + (voiceIndex * 0x10);
	SetCoreRegister(coreIndex, voiceAddress, value);
}

void CTest::SetVoiceAddress(unsigned int coreIndex, uint32 voiceIndex, uint32 registerAddress, uint32 targetAddress)
{
	uint32 voiceAddress = registerAddress + (voiceIndex * 12);
	SetCoreAddress(coreIndex, voiceAddress, targetAddress);
}
