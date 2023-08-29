#include "SimpleIrqTest.h"

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
	for(unsigned int i = 0; i < VOICE_COUNT; i++)
	{
		SetVoiceRegister(0, i, Iop::Spu2::CCore::VP_PITCH, 0x1000);
		SetVoiceAddress(0, i, Iop::Spu2::CCore::VA_SSA_HI, nullSampleAddress);
	}

	//Send KEY-ON
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0xFFFF);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_LO, 0xFF);

	//Set IRQ parameters (set IRQA to end of RAM to avoid triggering from CORE0_SIN region)
	SetCoreRegister(0, Iop::Spu2::CCore::CORE_ATTR, Iop::CSpuBase::CONTROL_IRQ);
	SetCoreAddress(0, Iop::Spu2::CCore::A_IRQA_HI, ~0);

	RunSpu(1024);

	//No IRQ
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0);
	}

	//Set voice 0 address to IRQ address
	SetCoreAddress(0, Iop::Spu2::CCore::A_IRQA_HI, irqSampleAddress);
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, irqSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0x1);

	RunSpu(1024);

	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0x04);
	}
}
