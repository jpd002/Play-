#include "SimpleIrqTest.h"
#include "Ps2Const.h"
#include <vector>

CSimpleIrqTest::CSimpleIrqTest()
: m_ram(new uint8[PS2::SPU_RAM_SIZE])
, m_spuCore0(m_ram, PS2::SPU_RAM_SIZE, 0)
, m_spuCore1(m_ram, PS2::SPU_RAM_SIZE, 1)
, m_spu(m_spuCore0, m_spuCore1)
{
	memset(m_ram, 0, PS2::SPU_RAM_SIZE);
}

CSimpleIrqTest::~CSimpleIrqTest()
{
	delete [] m_ram;
}

void CSimpleIrqTest::Execute()
{
	static uint32 nullSampleAddress = 0x5000;
	static uint32 irqSampleAddress = 0x6000;
	
	//Set some samples
	m_ram[nullSampleAddress + 0] = 0;
	m_ram[nullSampleAddress + 1] = 0x07;
	
	m_ram[irqSampleAddress + 0] = 0;
	m_ram[irqSampleAddress + 1] = 0x07;
	
	//Setup voices
	for(unsigned int i = 0; i < 24; i++)
	{
		SetVoiceRegister(0, i, Iop::Spu2::CCore::VP_PITCH, 0x1000);
		SetVoiceAddress(0, i, Iop::Spu2::CCore::VA_SSA_HI, nullSampleAddress);
	}
	
	//Send KEY-ON
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0xFFFF);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_LO, 0xFF);
	
	//Set IRQ parameters
	SetCoreRegister(0, Iop::Spu2::CCore::CORE_ATTR, Iop::CSpuBase::CONTROL_IRQ);
	SetCoreAddress(0, Iop::Spu2::CCore::A_IRQA_HI, 0x10);

	RunSpu(128);
	
	//No IRQ
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0);
	}
	
	//Set voice 0 address to IRQ address
	SetCoreAddress(0, Iop::Spu2::CCore::A_IRQA_HI, irqSampleAddress);
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, irqSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0x1);

	RunSpu(128);
	
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0x04);
	}
}

void CSimpleIrqTest::RunSpu(unsigned int ticks)
{
	static const unsigned int DST_SAMPLE_RATE = 44100;
	unsigned int blockSize = ticks * 2;
	std::vector<int16> samples(blockSize);
	m_spuCore0.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
	m_spuCore1.Render(samples.data(), blockSize, DST_SAMPLE_RATE);
}

void CSimpleIrqTest::SetCoreRegister(unsigned int coreIndex, uint32 address, uint32 value)
{
	assert(coreIndex == 0);
	m_spu.WriteRegister(address, value);
}

void CSimpleIrqTest::SetCoreAddress(unsigned int coreIndex, uint32 registerAddress, uint32 targetAddress)
{
	assert((registerAddress & 0x2) == 0);
	SetCoreRegister(coreIndex, registerAddress + 0, (targetAddress >> 17) & 0xFFFE);
	SetCoreRegister(coreIndex, registerAddress + 2, (targetAddress >> 1) & 0xFFFE);
}

void CSimpleIrqTest::SetVoiceRegister(unsigned int coreIndex, uint32 voiceIndex, uint32 address, uint32 value)
{
	uint32 voiceAddress = address + (voiceIndex * 0x10);
	SetCoreRegister(coreIndex, voiceAddress, value);
}

void CSimpleIrqTest::SetVoiceAddress(unsigned int coreIndex, uint32 voiceIndex, uint32 registerAddress, uint32 targetAddress)
{
	uint32 voiceAddress = registerAddress + (voiceIndex * 12);
	SetCoreAddress(coreIndex, voiceAddress, targetAddress);
}
