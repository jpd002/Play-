#include "SetRepeatTest.h"

void CSetRepeatTest::Execute()
{
	static uint32 nullSampleAddress = 0x5000;
	static uint32 otherSampleAddress = 0x6000;

	//Set some samples
	m_ram[nullSampleAddress + 0] = 0;
	m_ram[nullSampleAddress + 1] = 0x07;

	m_ram[otherSampleAddress + 0] = 0;
	m_ram[otherSampleAddress + 1] = 0x01;

	//Setup voices
	for(unsigned int i = 0; i < VOICE_COUNT; i++)
	{
		SetVoiceRegister(0, i, Iop::Spu2::CCore::VP_PITCH, 0x1000);
		SetVoiceAddress(0, i, Iop::Spu2::CCore::VA_SSA_HI, nullSampleAddress);
	}

	//Send KEY-ON
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0xFFFF);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_LO, 0xFF);

	//Let the SPU run for a while
	RunSpu(128);

	{
		//At this point, since we have not written channel registers, repeat address should be
		//whatever we set the start address initially.

		uint32 repeatAddress = GetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI);
		TEST_VERIFY(repeatAddress == nullSampleAddress);
	}

	//Now, update the repeat address
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI, otherSampleAddress);

	RunSpu(128);

	{
		//Check that repeat address has not changed
		uint32 repeatAddress = GetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI);
		TEST_VERIFY(repeatAddress == otherSampleAddress);
	}

	//Set start address and send KEY-ON for our first voice
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, otherSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 1);

	RunSpu(128);

	{
		//Check that repeat address is still the same
		uint32 repeatAddress = GetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI);
		TEST_VERIFY(repeatAddress == otherSampleAddress);
	}
}
