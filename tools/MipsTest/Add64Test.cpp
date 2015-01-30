#include "Add64Test.h"
#include "MIPSAssembler.h"

void CAdd64Test::Execute(CTestVm& virtualMachine)
{
	const uint32 baseAddress = 0x100;
	const uint64 constantValue0 = 0xFFF0FFFFFFFFFFFFULL;
	const uint64 constantValue1 = 0x8000000000000001ULL;
	CMIPS::REGISTER valueRegister0 = CMIPS::A0;
	CMIPS::REGISTER valueRegister1 = CMIPS::A1;
	CMIPS::REGISTER addReg32Result = CMIPS::T0;
	CMIPS::REGISTER addReg64Result = CMIPS::T1;
	CMIPS::REGISTER addCst32Result = CMIPS::T2;
	CMIPS::REGISTER addCst64Result = CMIPS::T3;

	virtualMachine.Reset();

	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		assembler.ADDU(addReg32Result, valueRegister0, valueRegister1);
		assembler.DADDU(addReg64Result, valueRegister0, valueRegister1);
		assembler.ADDIU(addCst32Result, valueRegister0, constantValue1);
		assembler.DADDIU(addCst64Result, valueRegister0, constantValue1);

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[valueRegister0].nD0 = constantValue0;
	virtualMachine.m_cpu.m_State.nGPR[valueRegister1].nD0 = constantValue1;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister0].nD0 == constantValue0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister1].nD0 == constantValue1);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[addReg32Result].nD0 == static_cast<int32>(constantValue0 + constantValue1));
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[addReg64Result].nD0 == constantValue0 + constantValue1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[addCst32Result].nD0 == static_cast<int32>(constantValue0 + static_cast<int16>(constantValue1)));
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[addCst64Result].nD0 == constantValue0 + static_cast<int16>(constantValue1));
}
