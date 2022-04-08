#include "Test.h"

#include <vector>
#include <cassert>
#include <cstring>

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
	delete[] m_ram;
}

void CTest::RunSpu(unsigned int ticks)
{
	static const unsigned int DST_SAMPLE_RATE = 44100;
	unsigned int blockSize = ticks * 2;
	std::vector<int16> samples(blockSize);
	m_spuCore0.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
	m_spuCore1.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
}

uint32 CTest::GetCoreRegister(unsigned int coreIndex, uint32 address)
{
	assert(coreIndex == 0);
	return m_spu.ReadRegister(address);
}

void CTest::SetCoreRegister(unsigned int coreIndex, uint32 address, uint32 value)
{
	assert(coreIndex == 0);
	m_spu.WriteRegister(address, value);
}

uint32 CTest::GetCoreAddress(unsigned int coreIndex, uint32 registerAddress)
{
	assert((registerAddress & 0x2) == 0);
	uint32 addrHi = GetCoreRegister(coreIndex, registerAddress + 0);
	uint32 addrLo = GetCoreRegister(coreIndex, registerAddress + 2);
	return (addrHi << 17) | (addrLo << 1);
}

void CTest::SetCoreAddress(unsigned int coreIndex, uint32 registerAddress, uint32 targetAddress)
{
	assert((registerAddress & 0x2) == 0);
	SetCoreRegister(coreIndex, registerAddress + 0, (targetAddress >> 17) & 0xFFFE);
	SetCoreRegister(coreIndex, registerAddress + 2, (targetAddress >> 1) & 0xFFFE);
}

uint32 CTest::GetVoiceRegister(unsigned int coreIndex, uint32 voiceIndex, uint32 address)
{
	uint32 voiceAddress = address + (voiceIndex * 0x10);
	return GetCoreRegister(coreIndex, voiceAddress);
}

void CTest::SetVoiceRegister(unsigned int coreIndex, uint32 voiceIndex, uint32 address, uint32 value)
{
	uint32 voiceAddress = address + (voiceIndex * 0x10);
	SetCoreRegister(coreIndex, voiceAddress, value);
}

uint32 CTest::GetVoiceAddress(unsigned int coreIndex, unsigned int voiceIndex, uint32 registerAddress)
{
	uint32 voiceAddress = registerAddress + (voiceIndex * 12);
	return GetCoreAddress(coreIndex, voiceAddress);
}

void CTest::SetVoiceAddress(unsigned int coreIndex, unsigned int voiceIndex, uint32 registerAddress, uint32 targetAddress)
{
	uint32 voiceAddress = registerAddress + (voiceIndex * 12);
	SetCoreAddress(coreIndex, voiceAddress, targetAddress);
}
