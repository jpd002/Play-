#include "IntBranchDelayTest2.h"
#include "VuAssembler.h"

void CIntBranchDelayTest2::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Lego Star Wars

	{
		CVuAssembler assembler(microMem);

		auto notEqualLabel = assembler.CreateLabel();
		auto doneLabel = assembler.CreateLabel();

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::LQI(CVuAssembler::DEST_XYZW, CVuAssembler::VF31, CVuAssembler::VI3));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::LQI(CVuAssembler::DEST_XYZW, CVuAssembler::VF30, CVuAssembler::VI3));

		//Here, the value of VI3 picked up by IBNE should be the value prior to the change from LQI
		//since there is no delay between the branch instruction and the register changing instruction.
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBNE(CVuAssembler::VI3, CVuAssembler::VI2, notEqualLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI1, CVuAssembler::VI0, 0));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI0, doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.MarkLabel(notEqualLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI1, CVuAssembler::VI0, 1));

		assembler.MarkLabel(doneLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	//Fuzz savedIntReg to make sure it's properly modified
	virtualMachine.m_cpu.m_State.savedIntReg = 6;
	virtualMachine.m_cpu.m_State.nCOP2VI[2] = 6;
	virtualMachine.m_cpu.m_State.nCOP2VI[3] = 4;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[1] == 1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[2] == 6);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[3] == 6);
}
