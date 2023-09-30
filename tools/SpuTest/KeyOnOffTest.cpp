#include "KeyOnOffTest.h"

void CKeyOnOffTest::Execute()
{
	//Loosely based on Black
	//The game will send a KON and KOFF sequence to update the voice addresses
	//Since the game is enabling IRQ checks, if we don't handle the change properly
	//the SPU might trigger IRQs because the voice address has not been updated

	static unsigned int testCoreIndex = 0;
	static unsigned int testVoiceIndex = 0;
	static uint32 sampleAddress1 = 0x5000;
	static uint32 sampleAddress2 = 0x6000;

	//Set some samples
	m_ram[sampleAddress1 + 0] = 0;
	m_ram[sampleAddress1 + 1] = 0x07;

	m_ram[sampleAddress2 + 0] = 0;
	m_ram[sampleAddress2 + 1] = 0x07;

	//Setup voices
	for(unsigned int i = 0; i < VOICE_COUNT; i++)
	{
		SetVoiceRegister(testCoreIndex, i, Iop::Spu2::CCore::VP_PITCH, 0x1000);
		SetVoiceAddress(testCoreIndex, i, Iop::Spu2::CCore::VA_SSA_HI, sampleAddress1);
	}

	//Send KEY-ON
	SetCoreRegister(testCoreIndex, Iop::Spu2::CCore::A_KON_HI, 0xFFFF);
	SetCoreRegister(testCoreIndex, Iop::Spu2::CCore::A_KON_LO, 0xFF);

	RunSpu(64);

	//We sent a KEY-ON, current address should be what we set as start address
	{
		uint32 testVoiceCurrAddr = GetVoiceAddress(testCoreIndex, testVoiceIndex, Iop::Spu2::CCore::VA_NAX_HI);
		TEST_VERIFY((testVoiceCurrAddr & ~0xF) == (sampleAddress1 & ~0xF));
	}

	//Change start address and then send KEY-ON and KEY-OFF right after
	SetVoiceAddress(testCoreIndex, testVoiceIndex, Iop::Spu2::CCore::VA_SSA_HI, sampleAddress2);
	SetCoreRegister(testCoreIndex, Iop::Spu2::CCore::A_KON_HI, (1 << testVoiceIndex));
	SetCoreRegister(testCoreIndex, Iop::Spu2::CCore::A_KOFF_HI, (1 << testVoiceIndex));

	RunSpu(64);

	//After a quick key-on and key-off sequence, the current voice address should have been updated
	//On a real console, I think this would take a few cycles, but since we don't update the SPU
	//all the time, we should be able to handle this even though the update timestep is not small
	{
		uint32 testVoiceCurrAddr = GetVoiceAddress(testCoreIndex, testVoiceIndex, Iop::Spu2::CCore::VA_NAX_HI);
		TEST_VERIFY((testVoiceCurrAddr & ~0xF) == (sampleAddress2 & ~0xF));
	}

	//Enable IRQs and check that our voice's address is still good
	//TODO: Check if this still actually needed. Black sets IRQA inside CORE1_SIN areas.
	//Maybe that test was made before we updated voices all the time and not just when we had an IRQA set
	SetCoreRegister(testCoreIndex, Iop::Spu2::CCore::CORE_ATTR, Iop::CSpuBase::CONTROL_IRQ);
	SetCoreAddress(testCoreIndex, Iop::Spu2::CCore::A_IRQA_HI, ~0);

	RunSpu(1024);

	{
		uint32 irqInfo = m_spu.ReadRegister(Iop::CSpu2::C_IRQINFO);
		TEST_VERIFY(irqInfo == 0);

		uint32 testVoiceCurrAddr = GetVoiceAddress(testCoreIndex, testVoiceIndex, Iop::Spu2::CCore::VA_NAX_HI);
		TEST_VERIFY((testVoiceCurrAddr & ~0xF) == (sampleAddress2 & ~0xF));
	}
}
