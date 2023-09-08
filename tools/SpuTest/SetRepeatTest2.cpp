#include "SetRepeatTest2.h"

void CSetRepeatTest2::Execute()
{
	//Inspired by Chaos Legion. Sets LSAX quickly after key-on.
	//Sample has bit to set repeat address, but must not be taken in consideration.

	static uint32 nullSampleAddress = 0x5000;
	static uint32 loopingSampleAddress = 0x6000;

	//Set some samples
	m_ram[nullSampleAddress + 0] = 0;
	m_ram[nullSampleAddress + 1] = 0x07; //Turn off voice

	m_ram[loopingSampleAddress + 0] = 0;
	m_ram[loopingSampleAddress + 1] = 0;

	m_ram[loopingSampleAddress + 0x10 + 0] = 0;
	m_ram[loopingSampleAddress + 0x10 + 1] = 0x04; //Set loop point

	m_ram[loopingSampleAddress + 0x20 + 0] = 0;
	m_ram[loopingSampleAddress + 0x20 + 1] = 0x03; //Repeat

	//Test setting loop info from the sample first
	SetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_PITCH, 0x1000);
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, loopingSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0x1);
	RunSpu(1024);

	//Check that loop address is set
	{
		uint32 loopAddress = GetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI);
		TEST_VERIFY(loopAddress == (loopingSampleAddress + 0x10));
	}

	//Check ENDX
	{
		uint32 endX = GetCoreRegister(0, Iop::Spu2::CCore::S_ENDX_HI);
		TEST_VERIFY(endX == 1);
	}

	//Turn off voice
	SetCoreRegister(0, Iop::Spu2::CCore::A_KOFF_HI, 0x1);
	RunSpu(1024);

	//This time, same voice setup, but set LSAX right after keying-on
	SetVoiceRegister(0, 0, Iop::Spu2::CCore::VP_PITCH, 0x1000);
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_SSA_HI, loopingSampleAddress);
	SetCoreRegister(0, Iop::Spu2::CCore::A_KON_HI, 0x1);
	SetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI, nullSampleAddress);
	RunSpu(1024);

	//Check that loop address is now what we set explicitly
	{
		uint32 loopAddress = GetVoiceAddress(0, 0, Iop::Spu2::CCore::VA_LSAX_HI);
		TEST_VERIFY(loopAddress == nullSampleAddress);
	}
}
