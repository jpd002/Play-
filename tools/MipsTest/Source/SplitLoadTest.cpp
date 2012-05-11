#include "SplitLoadTest.h"

void CSplitLoadTest::Execute(CTestVm& virtualMachine)
{
	const uint32 baseAddress = 0x100;
	const uint32 addressValue = 0x80;
	const uint32 constant32Value0 = 0x30201000;
	const uint32 constant32Value1 = 0x70605040;
	const uint64 constant64Value0 = 0x0706050403020100ULL;
	const uint64 constant64Value1 = 0x0F0E0D0C0B0A0908ULL;

	CMIPS::REGISTER addressRegister = CMIPS::A0;
	CMIPS::REGISTER value32Register = CMIPS::A1;
	CMIPS::REGISTER value64Register = CMIPS::A2;

	CMIPS::REGISTER result32Register = CMIPS::T0;
	CMIPS::REGISTER result64Register = CMIPS::T1;

	virtualMachine.Reset();

	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x00, constant32Value0);
	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x04, constant32Value1);

	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x08, static_cast<uint32>(constant64Value0 >>  0));
	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x0C, static_cast<uint32>(constant64Value0 >> 32));
	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x10, static_cast<uint32>(constant64Value1 >>  0));
	virtualMachine.m_cpu.m_pMemoryMap->SetWord(addressValue + 0x14, static_cast<uint32>(constant64Value1 >> 32));

	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		assembler.LWL(result32Register, 0x3 + 2, addressRegister);
		assembler.LWR(result32Register, 0x0 + 2, addressRegister);

		assembler.LDL(result64Register, 0xF + 4, addressRegister);
		assembler.LDR(result64Register, 0x8 + 4, addressRegister);

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[addressRegister].nD0 = addressValue;
	virtualMachine.m_cpu.m_State.nGPR[value32Register].nD0 = static_cast<int32>(constant32Value0);
	virtualMachine.m_cpu.m_State.nGPR[value64Register].nD0 = constant64Value0;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[addressRegister].nD0 == addressValue);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[result32Register].nD0 == 0x50403020);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[result64Register].nD0 == 0x0B0A090807060504);
}
