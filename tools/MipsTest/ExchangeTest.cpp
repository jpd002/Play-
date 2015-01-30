#include "ExchangeTest.h"
#include "EEAssembler.h"

void CExchangeTest::Execute(CTestVm& virtualMachine)
{
	auto& cpu = virtualMachine.m_cpu;

	const uint32 baseAddress = 0x100;
	const uint32 constantValue0 = 0x00000000;
	const uint32 constantValue1 = 0x11111111;
	const uint32 constantValue2 = 0x22222222;
	const uint32 constantValue3 = 0x33333333;
	const uint64 exchResultLow  = 0x1111000011110000ULL;
	const uint64 exchResultHigh = 0x3333222233332222ULL;
	const uint64 excwResultLow  = 0x2222222200000000ULL;
	const uint64 excwResultHigh = 0x3333333311111111ULL;

	auto valueRegister	= CMIPS::A0;
	auto exchResult		= CMIPS::T0;
	auto exchSelfResult	= CMIPS::T1;
	auto excwResult		= CMIPS::T2;
	auto excwSelfResult	= CMIPS::T3;

	virtualMachine.Reset();

	{
		CEEAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		//PEXCH
		assembler.PEXCH(exchResult, valueRegister);

		//PEXCH (self)
		assembler.PADDW(exchSelfResult, valueRegister, CMIPS::R0);
		assembler.PEXCH(exchSelfResult, exchSelfResult);

		//PEXCW
		assembler.PEXCW(excwResult, valueRegister);

		//PEXCW (self)
		assembler.PADDW(excwSelfResult, valueRegister, CMIPS::R0);
		assembler.PEXCW(excwSelfResult, excwSelfResult);

		assembler.SYSCALL();
	}

	//Setup initial state
	cpu.m_State.nGPR[valueRegister].nV[0] = constantValue0;
	cpu.m_State.nGPR[valueRegister].nV[1] = constantValue1;
	cpu.m_State.nGPR[valueRegister].nV[2] = constantValue2;
	cpu.m_State.nGPR[valueRegister].nV[3] = constantValue3;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister].nV[0] == constantValue0);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister].nV[1] == constantValue1);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister].nV[2] == constantValue2);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister].nV[3] == constantValue3);

	TEST_VERIFY(cpu.m_State.nGPR[exchResult].nD0 == exchResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[exchResult].nD1 == exchResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[exchSelfResult].nD0 == exchResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[exchSelfResult].nD1 == exchResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[excwResult].nD0 == excwResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[excwResult].nD1 == excwResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[excwSelfResult].nD0 == excwResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[excwSelfResult].nD1 == excwResultHigh);
}
