#include "MultiCoreIrqTest.h"

void CMultiCoreIrqTest::Execute()
{
	static uint32 nullSampleAddress = 0x5000;
	static uint32 core0IrqSampleAddress = 0x6000;
	static uint32 core1IrqSampleAddress = 0x7000;

	//Set some samples
	m_ram[nullSampleAddress + 0] = 0;
	m_ram[nullSampleAddress + 1] = 0x07;

	m_ram[core0IrqSampleAddress + 0] = 0;
	m_ram[core0IrqSampleAddress + 1] = 0x07;

	m_ram[core1IrqSampleAddress + 0] = 0;
	m_ram[core1IrqSampleAddress + 1] = 0x07;

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
	for(int i = 0; i < CORE_COUNT; i++)
	{
		SetCoreRegister(i, Iop::Spu2::CCore::CORE_ATTR, Iop::CSpuBase::CONTROL_IRQ);
		SetCoreAddress(i, Iop::Spu2::CCore::A_IRQA_HI, ~0);
	}

	RunSpu(1024);

	//No IRQ
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0);
	}

	//Set IRQ addresses
	SetCoreAddress(0, Iop::Spu2::CCore::A_IRQA_HI, core0IrqSampleAddress);
	SetCoreAddress(1, Iop::Spu2::CCore::A_IRQA_HI, core1IrqSampleAddress);

	//Set CORE0 voice 0 address to CORE1 IRQ address
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, core1IrqSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0x1);

	RunSpu(1024);

	//Check that IRQ has been triggered on CORE1 even though no voice is playing there
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0x08);
	}

	//Disable interrupts on CORE1
	uint32 attr = GetCoreRegister(1, Iop::Spu2::CCore::CORE_ATTR);
	SetCoreRegister(1, Iop::Spu2::CCore::CORE_ATTR, attr & ~Iop::CSpuBase::CONTROL_IRQ);
	SetCoreAddress(1, Iop::Spu2::CCore::A_IRQA_HI, ~0);

	RunSpu(1024);

	//No IRQ
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0);
	}

	//Set CORE1 voice 0 address to CORE0 IRQ address
	SetVoiceAddress(1, 0, Iop::Spu2::CCore::VA_SSA_HI, core0IrqSampleAddress);
	SetCoreRegister(1, Iop::Spu2::CCore::A_KON_HI, 0x1);

	//Run two slices to let CORE0 pick up the update from CORE1
	RunSpu(512);
	RunSpu(512);

	//Check that IRQ has been triggered on CORE0 even though no voice is playing there
	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0x04);
	}
}
