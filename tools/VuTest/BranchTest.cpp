#include "BranchTest.h"
#include "VuAssembler.h"

void CBranchTest::Execute(CTestVm& virtualMachine)
{
	//Inspired by Star Ocean 3
	auto branch1HitRegister = CVuAssembler::VI8;
	auto branch2HitRegister = CVuAssembler::VI9;
	auto nextBlockAddress = 0u;
	auto branch1Address = 0u;
	auto branch2Address = 0u;

	virtualMachine.Reset();

	{
		auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);
		CVuAssembler assembler(microMem);

		auto branch1Label = assembler.CreateLabel();
		auto branch2Label = assembler.CreateLabel();

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(branch1Label));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI2, CVuAssembler::VI0, branch2Label));

		nextBlockAddress = assembler.GetProgramSize() * CVuAssembler::INSTRUCTION_SIZE;

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//Branch 1
		assembler.MarkLabel(branch1Label);
		branch1Address = assembler.GetProgramSize() * CVuAssembler::INSTRUCTION_SIZE;

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(branch1HitRegister, CVuAssembler::VI0, 1));

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//Branch 2
		assembler.MarkLabel(branch2Label);
		branch2Address = assembler.GetProgramSize() * CVuAssembler::INSTRUCTION_SIZE;

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(branch2HitRegister, CVuAssembler::VI0, 1));

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	//Execute all possible blocks to make sure all possible links are here
	virtualMachine.ExecuteTest(0);
	virtualMachine.ExecuteTest(nextBlockAddress);
	virtualMachine.ExecuteTest(branch1Address);
	virtualMachine.ExecuteTest(branch2Address);

	//If VI2 is zero, should end up in branch 2 and we executed the starting instruction of branch 1
	virtualMachine.m_cpu.m_State.nCOP2VI[2] = 0;
	virtualMachine.m_cpu.m_State.nCOP2VI[branch1HitRegister] = 0;
	virtualMachine.m_cpu.m_State.nCOP2VI[branch2HitRegister] = 0;
	virtualMachine.ExecuteTest(0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[branch1HitRegister] == 1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[branch2HitRegister] == 1);

	//If VI2 not zero, should end up in branch 1
	virtualMachine.m_cpu.m_State.nCOP2VI[2] = 1;
	virtualMachine.m_cpu.m_State.nCOP2VI[branch1HitRegister] = 0;
	virtualMachine.m_cpu.m_State.nCOP2VI[branch2HitRegister] = 0;
	virtualMachine.ExecuteTest(0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[branch1HitRegister] == 1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[branch2HitRegister] == 0);
}
