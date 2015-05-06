#include "PackTest.h"
#include "ee/EEAssembler.h"

void CPackTest::Execute(CTestVm& virtualMachine)
{
	auto& cpu = virtualMachine.m_cpu;

	const uint32 baseAddress = 0x100;
	const uint32 constantValue0 = 0x00000000;
	const uint32 constantValue1 = 0x11111111;
	const uint32 constantValue2 = 0x22222222;
	const uint32 constantValue3 = 0x33333333;
	const uint32 constantValue4 = 0x44444444;
	const uint32 constantValue5 = 0x55555555;
	const uint32 constantValue6 = 0x66666666;
	const uint32 constantValue7 = 0x77777777;
	const uint64 pachResultLow  = 0x7777666655554444ULL;
	const uint64 pachResultHigh = 0x3333222211110000ULL;
	const uint64 pacwResultLow  = 0x6666666644444444ULL;
	const uint64 pacwResultHigh = 0x2222222200000000ULL;

	auto valueRegister0	= CMIPS::A0;
	auto valueRegister1 = CMIPS::A1;
	auto pachResult		= CMIPS::T0;
	auto pachSelfResult	= CMIPS::T1;
	auto pacwResult		= CMIPS::T2;
	auto pacwSelfResult	= CMIPS::T3;

	virtualMachine.Reset();

	{
		CEEAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		//PPACH
		assembler.PPACH(pachResult, valueRegister0, valueRegister1);

		//PPACH (self)
		assembler.PADDW(pachSelfResult, valueRegister0, CMIPS::R0);
		assembler.PPACH(pachSelfResult, pachSelfResult, valueRegister1);

		//PPACW
		assembler.PPACW(pacwResult, valueRegister0, valueRegister1);

		//PPACW (self)
		assembler.PADDW(pacwSelfResult, valueRegister0, CMIPS::R0);
		assembler.PPACW(pacwSelfResult, pacwSelfResult, valueRegister1);

		assembler.SYSCALL();
	}

	//Setup initial state
	cpu.m_State.nGPR[valueRegister0].nV[0] = constantValue0;
	cpu.m_State.nGPR[valueRegister0].nV[1] = constantValue1;
	cpu.m_State.nGPR[valueRegister0].nV[2] = constantValue2;
	cpu.m_State.nGPR[valueRegister0].nV[3] = constantValue3;

	cpu.m_State.nGPR[valueRegister1].nV[0] = constantValue4;
	cpu.m_State.nGPR[valueRegister1].nV[1] = constantValue5;
	cpu.m_State.nGPR[valueRegister1].nV[2] = constantValue6;
	cpu.m_State.nGPR[valueRegister1].nV[3] = constantValue7;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister0].nV[0] == constantValue0);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister0].nV[1] == constantValue1);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister0].nV[2] == constantValue2);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister0].nV[3] == constantValue3);

	TEST_VERIFY(cpu.m_State.nGPR[valueRegister1].nV[0] == constantValue4);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister1].nV[1] == constantValue5);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister1].nV[2] == constantValue6);
	TEST_VERIFY(cpu.m_State.nGPR[valueRegister1].nV[3] == constantValue7);

	TEST_VERIFY(cpu.m_State.nGPR[pachResult].nD0 == pachResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[pachResult].nD1 == pachResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[pachSelfResult].nD0 == pachResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[pachSelfResult].nD1 == pachResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[pacwResult].nD0 == pacwResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[pacwResult].nD1 == pacwResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[pacwSelfResult].nD0 == pacwResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[pacwSelfResult].nD1 == pacwResultHigh);
}
