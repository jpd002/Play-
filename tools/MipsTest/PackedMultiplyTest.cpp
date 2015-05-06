#include "PackedMultiplyTest.h"
#include "ee/EEAssembler.h"

void CPackedMultiplyTest::Execute(CTestVm& virtualMachine)
{
	auto& cpu = virtualMachine.m_cpu;

	const uint32 baseAddress = 0x100;
	const uint32 constantValue0		= 0x11110000;
	const uint32 constantValue1		= 0x33332222;
	const uint32 constantValue2		= 0x55554444;
	const uint32 constantValue3		= 0x77776666;
	const uint32 constantValue4		= 0x99998888;
	const uint32 constantValue5		= 0xBBBBAAAA;
	const uint32 constantValue6		= 0xDDDDCCCC;
	const uint32 constantValue7		= 0xFFFFEEEE;
	const uint64 multhResultLow		= 0xF49F3E9400000000ULL;
	const uint64 multhResultHigh	= 0xF92C06D4F2589630ULL;
	const uint64 multhLoLow			= 0xF92C5C2900000000ULL;
	const uint64 multhLoHigh		= 0xF49F0B61F2589630ULL;
	const uint64 multhHiLow			= 0xF258A741F49F3E94ULL;
	const uint64 multhHiHigh		= 0xFFFF8889F92C06D4ULL;
	const uint64 mfhlUwResultLow	= 0xF258A741F92C5C29ULL;
	const uint64 mfhlUwResultHigh	= 0xFFFF8889F49F0B61ULL;
	const uint64 mfhlLhResultLow	= 0xA7413E945C290000ULL;
	const uint64 mfhlLhResultHigh	= 0x888906D40B619630ULL;

	auto valueRegister0		= CMIPS::A0;
	auto valueRegister1		= CMIPS::A1;
	auto multhResult		= CMIPS::T0;
	auto multhLo			= CMIPS::T1;
	auto multhHi			= CMIPS::T2;
	auto multhSelfResult	= CMIPS::T3;
	auto multhSelfLo		= CMIPS::T4;
	auto multhSelfHi		= CMIPS::T5;
	auto mfhlUwResult		= CMIPS::T6;
	auto mfhlLhResult		= CMIPS::T7;

	virtualMachine.Reset();

	{
		CEEAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		//PMULTH
		assembler.PMULTH(multhResult, valueRegister0, valueRegister1);
		assembler.PMFLO(multhLo);
		assembler.PMFHI(multhHi);

		//PMULTH (self)
		assembler.PADDW(multhSelfResult, valueRegister0, CMIPS::R0);
		assembler.PMULTH(multhSelfResult, multhSelfResult, valueRegister1);
		assembler.PMFLO(multhSelfLo);
		assembler.PMFHI(multhSelfHi);

		//PMFHL.UW
		assembler.PMFHL_UW(mfhlUwResult);

		//PMFHL.LH
		assembler.PMFHL_LH(mfhlLhResult);

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

	TEST_VERIFY(cpu.m_State.nGPR[multhResult].nD0 == multhResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhResult].nD1 == multhResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[multhLo].nD0 == multhLoLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhLo].nD1 == multhLoHigh);

	TEST_VERIFY(cpu.m_State.nGPR[multhHi].nD0 == multhHiLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhHi].nD1 == multhHiHigh);

	TEST_VERIFY(cpu.m_State.nGPR[multhSelfResult].nD0 == multhResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhSelfResult].nD1 == multhResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[multhSelfLo].nD0 == multhLoLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhSelfLo].nD1 == multhLoHigh);

	TEST_VERIFY(cpu.m_State.nGPR[multhSelfHi].nD0 == multhHiLow);
	TEST_VERIFY(cpu.m_State.nGPR[multhSelfHi].nD1 == multhHiHigh);

	TEST_VERIFY(cpu.m_State.nGPR[mfhlUwResult].nD0 == mfhlUwResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[mfhlUwResult].nD1 == mfhlUwResultHigh);

	TEST_VERIFY(cpu.m_State.nGPR[mfhlLhResult].nD0 == mfhlLhResultLow);
	TEST_VERIFY(cpu.m_State.nGPR[mfhlLhResult].nD1 == mfhlLhResultHigh);
}
