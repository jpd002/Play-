#include "SetLessThanTest.h"
#include "MIPSAssembler.h"

void CSetLessThanTest::Execute(CTestVm& virtualMachine)
{
	const uint32 baseAddress = 0x100;
	const uint64 constantValue0 = 0xFFFFFFFFFFFF0000ULL;
	const uint64 constantValue1 = 0x00000000FFFF0000ULL;
	const uint64 constantValue2 = 0xFFFFFFFF00000000ULL;
	CMIPS::REGISTER valueRegister0 = CMIPS::A0;
	CMIPS::REGISTER valueRegister1 = CMIPS::A1;
	CMIPS::REGISTER valueRegister2 = CMIPS::A2;

	CMIPS::REGISTER resultRegister0 = CMIPS::T0;
	CMIPS::REGISTER resultRegister1 = CMIPS::T1;
	CMIPS::REGISTER resultRegister2 = CMIPS::T2;
	CMIPS::REGISTER resultRegister3 = CMIPS::T3;
	CMIPS::REGISTER resultRegister4 = CMIPS::T4;
	CMIPS::REGISTER resultRegister5 = CMIPS::T5;

	virtualMachine.Reset();

	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		assembler.SLTU(resultRegister0, valueRegister0, valueRegister1);
		assembler.SLTU(resultRegister1, valueRegister1, valueRegister2);
		assembler.SLTU(resultRegister2, valueRegister2, valueRegister0);

		assembler.SLT(resultRegister3, valueRegister0, valueRegister1);
		assembler.SLT(resultRegister4, valueRegister1, valueRegister2);
		assembler.SLT(resultRegister5, valueRegister2, valueRegister0);

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[valueRegister0].nD0 = constantValue0;
	virtualMachine.m_cpu.m_State.nGPR[valueRegister1].nD0 = constantValue1;
	virtualMachine.m_cpu.m_State.nGPR[valueRegister2].nD0 = constantValue2;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister0].nD0 == constantValue0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister1].nD0 == constantValue1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister2].nD0 == constantValue2);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister0].nD0 == static_cast<uint64>(constantValue0) < static_cast<uint64>(constantValue1) ? 1 : 0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister1].nD0 == static_cast<uint64>(constantValue1) < static_cast<uint64>(constantValue2) ? 1 : 0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister2].nD0 == static_cast<uint64>(constantValue2) < static_cast<uint64>(constantValue0) ? 1 : 0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister3].nD0 == static_cast<int64>(constantValue0) < static_cast<int64>(constantValue1) ? 1 : 0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister4].nD0 == static_cast<int64>(constantValue1) < static_cast<int64>(constantValue2) ? 1 : 0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegister5].nD0 == static_cast<int64>(constantValue2) < static_cast<int64>(constantValue0) ? 1 : 0);
}
