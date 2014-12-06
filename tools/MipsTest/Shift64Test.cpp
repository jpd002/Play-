#include "Shift64Test.h"
#include "MIPSAssembler.h"

void CShift64Test::Execute(CTestVm& virtualMachine)
{
	TestShift(virtualMachine, &CMIPSAssembler::DSLL, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value << shiftAmount; });
	TestShift(virtualMachine, &CMIPSAssembler::DSRL, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value >> shiftAmount; });
	TestShift(virtualMachine, &CMIPSAssembler::DSRA, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return static_cast<int64>(value) >> shiftAmount; });
	TestShift(virtualMachine, &CMIPSAssembler::DSLL32, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value << (shiftAmount + 32); });
	TestShift(virtualMachine, &CMIPSAssembler::DSRL32, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value >> (shiftAmount + 32); });
	TestShift(virtualMachine, &CMIPSAssembler::DSRA32, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return static_cast<int64>(value) >> (shiftAmount + 32); });

	TestVariableShift(virtualMachine, &CMIPSAssembler::DSLLV, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value << shiftAmount; });
	TestVariableShift(virtualMachine, &CMIPSAssembler::DSRLV, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return value >> shiftAmount; });
	TestVariableShift(virtualMachine, &CMIPSAssembler::DSRAV, 
		[](uint64 value, unsigned int shiftAmount) -> uint64 { return static_cast<int64>(value) >> shiftAmount; });
}

void CShift64Test::TestShift(CTestVm& virtualMachine, const ShiftAssembleFunction& assembleFunction, const ShiftFunction& shiftFunction)
{
	const uint32 baseAddress = 0x100;
	const uint64 constantValue = 0x8080FFFF000080FFULL;
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
	virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 = constantValue;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 == constantValue);
	for(unsigned int i = 0; i < resultCount; i++)
	{
		uint64 result = shiftFunction(constantValue, i * 4);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegisters[i]].nD0 == result);
	}
}

void CShift64Test::TestVariableShift(CTestVm& virtualMachine, const VariableShiftAssembleFunction& assembleFunction, const ShiftFunction& shiftFunction)
{
	const uint32 baseAddress = 0x100;
	const uint64 constantValue = 0x8080FFFF000080FFULL;
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
			assembler.ADDIU(shiftAmountRegister, CMIPS::R0, i * 8);
			((assembler).*(assembleFunction))(resultRegisters[i], valueRegister, shiftAmountRegister);
		}

		assembler.SYSCALL();
	}

	//Setup initial state
	virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 = constantValue;

	//Execute
	virtualMachine.ExecuteTest(baseAddress);

	//Check final state
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[valueRegister].nD0 == constantValue);
	for(unsigned int i = 0; i < resultCount; i++)
	{
		uint64 result = shiftFunction(constantValue, i * 8);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nGPR[resultRegisters[i]].nD0 == result);
	}
}
