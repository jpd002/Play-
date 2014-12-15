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
	const uint64 rdHigh = 0x7777FFFF6666EEEEULL;
	const uint64 rdLow  = 0x5555DDDD4444CCCCULL;
	const uint64 rd2High = 0x3333BBBB2222AAAAULL;
	const uint64 rd2Low  = 0x1111999900008888ULL;

	auto rs = CMIPS::A0;
	auto rt = CMIPS::A1;
	auto rd = CMIPS::T0;
	auto rd2 = CMIPS::T1;

	virtualMachine.Reset();

	{
		CEEAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		//PEXCH
		assembler.PEXTUH(rd, rs, rt);
		assembler.PEXTLH(rd2, rs, rt);

		assembler.SYSCALL();
	}

	//Setup initial state
	cpu.m_State.nGPR[rs].nV[0] = constantValue0;
	cpu.m_State.nGPR[rs].nV[1] = constantValue1;
	cpu.m_State.nGPR[rs].nV[2] = constantValue2;
	cpu.m_State.nGPR[rs].nV[3] = constantValue3;
	cpu.m_State.nGPR[rt].nV[0] = constantValue4;
	cpu.m_State.nGPR[rt].nV[1] = constantValue5;
	cpu.m_State.nGPR[rt].nV[2] = constantValue6;
	cpu.m_State.nGPR[rt].nV[3] = constantValue7;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(cpu.m_State.nGPR[rs].nV[0] == constantValue0);
	TEST_VERIFY(cpu.m_State.nGPR[rs].nV[1] == constantValue1);
	TEST_VERIFY(cpu.m_State.nGPR[rs].nV[2] == constantValue2);
	TEST_VERIFY(cpu.m_State.nGPR[rs].nV[3] == constantValue3);
	TEST_VERIFY(cpu.m_State.nGPR[rt].nV[0] == constantValue4);
	TEST_VERIFY(cpu.m_State.nGPR[rt].nV[1] == constantValue5);
	TEST_VERIFY(cpu.m_State.nGPR[rt].nV[2] == constantValue6);
	TEST_VERIFY(cpu.m_State.nGPR[rt].nV[3] == constantValue7);

	TEST_VERIFY(cpu.m_State.nGPR[rd].nD0 == rdLow);
	TEST_VERIFY(cpu.m_State.nGPR[rd].nD1 == rdHigh);
	TEST_VERIFY(cpu.m_State.nGPR[rd2].nD0 == rd2Low);
	TEST_VERIFY(cpu.m_State.nGPR[rd2].nD1 == rd2High);
}
