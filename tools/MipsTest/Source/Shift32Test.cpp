#include "Shift32Test.h"
#include "MIPSAssembler.h"

void CShift32Test::Execute(CTestVm& virtualMachine)
{
	TestShift(virtualMachine, &CMIPSAssembler::SLL, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return value << shiftAmount; });
	TestShift(virtualMachine, &CMIPSAssembler::SRL, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return value >> shiftAmount; });
	TestShift(virtualMachine, &CMIPSAssembler::SRA, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return static_cast<int32>(value) >> shiftAmount; });

	TestVariableShift(virtualMachine, &CMIPSAssembler::SLLV, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return value << shiftAmount; });
	TestVariableShift(virtualMachine, &CMIPSAssembler::SRLV, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return value >> shiftAmount; });
	TestVariableShift(virtualMachine, &CMIPSAssembler::SRAV, 
		[](uint32 value, unsigned int shiftAmount) -> uint32 { return static_cast<int32>(value) >> shiftAmount; });
}

void CShift32Test::TestShift(CTestVm& virtualMachine, const ShiftAssembleFunction& assembleFunction, const ShiftFunction& shiftFunction)
{
	const uint32 baseAddress = 0x100;
	const uint32 constantValue = 0x808080FF;
	CMIPS::REGISTER valueRegister = CMIPS::A0;
	const unsigned int resultCount = 8;
	static const CMIPS::REGISTER resultRegisters[resultCount] =
	{
		CMIPS::T0,
		CMIPS::T1,
		CMIPS::T2,
		CMIPS::T3,
		CMIPS::T4,
		CMIPS::T5,
		CMIPS::T6,
		CMIPS::T7
	};

	virtualMachine.Reset();

	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		for(unsigned int i = 0; i < resultCount; i++)
		{
			((assembler).*(assembleFunction))(resultRegisters[i], valueRegister, i * 4);
		}

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 = static_cast<int32>(constantValue);

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister].nV0 == constantValue);
	for(unsigned int i = 0; i < resultCount; i++)
	{
		uint32 result = shiftFunction(constantValue, i * 4);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegisters[i]].nV0 == result);
	}
}

void CShift32Test::TestVariableShift(CTestVm& virtualMachine, const VariableShiftAssembleFunction& assembleFunction, const ShiftFunction& shiftFunction)
{
	const uint32 baseAddress = 0x100;
	const uint32 constantValue = 0x808080FF;
	CMIPS::REGISTER valueRegister = CMIPS::A0;
	CMIPS::REGISTER shiftAmountRegister = CMIPS::A1;
	const unsigned int resultCount = 8;
	static const CMIPS::REGISTER resultRegisters[resultCount] =
	{
		CMIPS::T0,
		CMIPS::T1,
		CMIPS::T2,
		CMIPS::T3,
		CMIPS::T4,
		CMIPS::T5,
		CMIPS::T6,
		CMIPS::T7
	};

	virtualMachine.Reset();

	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(virtualMachine.m_ram + baseAddress));

		for(unsigned int i = 0; i < resultCount; i++)
		{
			assembler.ADDIU(shiftAmountRegister, CMIPS::R0, i * 4);
			((assembler).*(assembleFunction))(resultRegisters[i], valueRegister, shiftAmountRegister);
		}

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 = static_cast<int32>(constantValue);

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister].nV0 == constantValue);
	for(unsigned int i = 0; i < resultCount; i++)
	{
		uint32 result = shiftFunction(constantValue, i * 4);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegisters[i]].nV0 == result);
	}
}
