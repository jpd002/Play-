#include "SweepTest.h"

void CSweepTest::Execute()
{
	//Just sample sweep test to make sure volume actually reduces and stays in bounds
	//Onimusha: Warlords relies on volume to reduce to 0

	static uint32 nullSampleAddress = 0x5000;

	//Set some samples (repeating)
	m_ram[nullSampleAddress + 0] = 0;
	m_ram[nullSampleAddress + 1] = 0x04;

	//Setup voices
	for(unsigned int i = 0; i < VOICE_COUNT; i++)
	{
		SetVoiceRegister(0, i, Iop::Spu2::CCore::VP_PITCH, 0x1000);
		SetVoiceAddress(0, i, Iop::Spu2::CCore::VA_SSA_HI, nullSampleAddress);
	}

	//Set left volume for channel 0 (fixed)
	SetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_VOLL, 0x3FFF);

	//Send KEY-ON for channel 0
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 1);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_LO, 0);

	RunSpu(128);

	//We just set the volume, make sure it's right, should be max value
	{
		uint32 vol = GetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_VOLXL);
		TEST_VERIFY(vol == 0x7FFE);
	}

	//Set left volume for channel 0 (sweep exp dec, 0x10 rate)
	SetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_VOLL, 0xE010);

	RunSpu(128 * 128);

	//Make sure we're at 0 now
	{
		uint32 vol = GetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_VOLXL);
		TEST_VERIFY(vol == 0);
	}
}
