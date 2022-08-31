#include "StallTest6.h"
#include "VuAssembler.h"

void CStallTest6::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Piposaru 2001 - ILW instruction causes a stall on integer register
	//Also involves a integer branch delay

	{
		CVuAssembler assembler(microMem);

		auto equalLabel = assembler.CreateLabel();
		auto doneLabel = assembler.CreateLabel();

		//pipeTime = 0 (ILW result at pipeTime 4)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ILW(CVuAssembler::DEST_X, CVuAssembler::VI1, 0, CVuAssembler::VI5));

		//pipeTime = 1
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI2, CVuAssembler::VI0, 0x22));

		//pipeTime = 2, 3, 4, branch should pick up updated VI2 register
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI2, CVuAssembler::VI1, equalLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI3, CVuAssembler::VI0, 0));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI0, doneLabel));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.MarkLabel(equalLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI3, CVuAssembler::VI0, 1));

		assembler.MarkLabel(doneLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	auto vuMem = reinterpret_cast<uint128*>(virtualMachine.m_vuMem);
	vuMem[0x10].nV0 = 0x22;

	virtualMachine.m_cpu.m_State.nCOP2VI[5] = 0x10;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[1] == 0x22);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[2] == 0x22);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[3] == 1);
}
