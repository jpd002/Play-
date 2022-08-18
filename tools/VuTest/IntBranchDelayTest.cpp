#include "IntBranchDelayTest.h"
#include "VuAssembler.h"

void CIntBranchDelayTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Dawn of Mana (super basic case)

	{
		CVuAssembler assembler(microMem);

		auto equalLabel = assembler.CreateLabel();
		auto doneLabel = assembler.CreateLabel();

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI1, CVuAssembler::VI1, 1));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI1, CVuAssembler::VI0, equalLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI2, CVuAssembler::VI0, 0));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI0, doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.MarkLabel(equalLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI2, CVuAssembler::VI0, 1));

		assembler.MarkLabel(doneLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	virtualMachine.m_cpu.m_State.nCOP2VI[1] = 1;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[1] == 0);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[2] == 1);
}
