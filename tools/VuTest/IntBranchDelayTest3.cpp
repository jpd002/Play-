#include "IntBranchDelayTest3.h"
#include "VuAssembler.h"

void CIntBranchDelayTest3::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Gauntlet: Seven Sorrows
	//Game decrements and tests a value in a sequence. We need to properly track
	//the actual tested value across basic blocks

	{
		CVuAssembler assembler(microMem);

		auto firstDecLabel = assembler.CreateLabel();
		auto secondDecLabel = assembler.CreateLabel();
		auto thirdDecLabel = assembler.CreateLabel();
		auto doneLabel = assembler.CreateLabel();

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI10, CVuAssembler::VI10, 1));

		//This should test value of initVI10
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI10, firstDecLabel));
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI10, CVuAssembler::VI10, 1));

		//This should test value of (initVI10 - 1)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI10, secondDecLabel));
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI10, CVuAssembler::VI10, 1));

		//This should test value of (initVI10 - 2)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI10, thirdDecLabel));
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ISUBIU(CVuAssembler::VI10, CVuAssembler::VI10, 1));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(doneLabel));
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI6, CVuAssembler::VI0, 0x00));

		assembler.MarkLabel(firstDecLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI6, CVuAssembler::VI0, 0x0F));

		assembler.MarkLabel(secondDecLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI6, CVuAssembler::VI0, 0xF0));

		assembler.MarkLabel(thirdDecLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI6, CVuAssembler::VI0, 0xFF));

		assembler.MarkLabel(doneLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	//Test Case 1
	{
		virtualMachine.m_cpu.m_State.nCOP2VI[10] = 0;
		virtualMachine.ExecuteTest(0);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[6] == 0x0F);
	}

	//Test Case 2
	{
		virtualMachine.m_cpu.m_State.nCOP2VI[10] = 1;
		virtualMachine.ExecuteTest(0);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[6] == 0xF0);
	}

	//Test Case 3
	{
		virtualMachine.m_cpu.m_State.nCOP2VI[10] = 2;
		virtualMachine.ExecuteTest(0);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[6] == 0xFF);
	}

	//Test Case 4
	{
		virtualMachine.m_cpu.m_State.nCOP2VI[10] = 3;
		virtualMachine.ExecuteTest(0);
		TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[6] == 0);
	}
}
