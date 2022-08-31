#include "IntBranchDelayTest.h"
#include "VuAssembler.h"

void CIntBranchDelayTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Dawn of Mana (FMAC delay right at the moment branch is to be executed)

	{
		CVuAssembler assembler(microMem);

		auto equalLabel = assembler.CreateLabel();
		auto doneLabel = assembler.CreateLabel();

		//pipeTime = 0 (LQ result available at pipeTime 4)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::LQ(CVuAssembler::DEST_XYZ, CVuAssembler::VF2, 0, CVuAssembler::VI5));

		//pipeTime = 1
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::LQ(CVuAssembler::DEST_XYZ, CVuAssembler::VF3, 1, CVuAssembler::VI5));

		//pipeTime = 2
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI1, CVuAssembler::VI1, 1));

		//pipeTime = 3, 4
		//Upper instruction stalls here waiting for result from pipeTime 0
		//This gives enough time for the IBEQ instruction to pick up the result
		//from the previous ISUBIU instruction
		assembler.Write(
		    CVuAssembler::Upper::ITOF12(CVuAssembler::DEST_XYZ, CVuAssembler::VF2, CVuAssembler::VF2),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI1, CVuAssembler::VI0, equalLabel));

		assembler.Write(
		    CVuAssembler::Upper::ITOF12(CVuAssembler::DEST_XYZ, CVuAssembler::VF3, CVuAssembler::VF3),
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
