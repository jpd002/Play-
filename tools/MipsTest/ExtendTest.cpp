#include "ExtendTest.h"
#include "EEAssembler.h"

void CExtendTest::Execute(CTestVm& virtualMachine)
{
	auto& cpu = virtualMachine.m_cpu;

	const uint32 baseAddress = 0x100;
	const uint32 constantValue0 = 0x11110000;
	const uint32 constantValue1 = 0x33332222;
	const uint32 constantValue2 = 0x55554444;
	const uint32 constantValue3 = 0x77776666;
	const uint32 constantValue4 = 0x99998888;
	const uint32 constantValue5 = 0xBBBBAAAA;
	const uint32 constantValue6 = 0xDDDDCCCC;
	const uint32 constantValue7 = 0xFFFFEEEE;
	const uint64 extlbResultLow  = 0x1199119900880088ULL;
	const uint64 extlbResultHigh = 0x33BB33BB22AA22AAULL;
	const uint64 extubResultLow  = 0x55DD55DD44CC44CCULL;
	const uint64 extubResultHigh = 0x77FF77FF66EE66EEULL;
	const uint64 extuhResultLow  = 0x5555DDDD4444CCCCULL;
	const uint64 extuhResultHigh = 0x7777FFFF6666EEEEULL;
	const uint64 extlhResultLow  = 0x1111999900008888ULL;
	const uint64 extlhResultHigh = 0x3333BBBB2222AAAAULL;

	auto valueRegister0 = CMIPS::A0;
	auto valueRegister1 = CMIPS::A1;
	auto extlbResult = CMIPS::T0;
	auto extubResult = CMIPS::T1;
	auto extlhResult = CMIPS::T2;
	auto extuhResult = CMIPS::T3;

	virtualMachine.Reset();

	{
		CEEAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		//PEXTLB
		assembler.PEXTLB(extlbResult, valueRegister0, valueRegister1);

		//PEXTUB
		assembler.PEXTUB(extubResult, valueRegister0, valueRegister1);

		//PEXTLH
		assembler.PEXTLH(extlhResult, valueRegister0, valueRegister1);

		//PEXTUH
		assembler.PEXTUH(extuhResult, valueRegister0, valueRegister1);

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

	TEST_VERIFY(cpu.m_State.nGPR[extlbResult].nD0 == extlbResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[extlbResult].nD1 == extlbResultHigh);
	TEST_VERIFY(cpu.m_State.nGPR[extubResult].nD0 == extubResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[extubResult].nD1 == extubResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[extlhResult].nD0 == extlhResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[extlhResult].nD1 == extlhResultHigh);
	TEST_VERIFY(cpu.m_State.nGPR[extuhResult].nD0 == extuhResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[extuhResult].nD1 == extuhResultHigh);
}
