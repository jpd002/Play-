#include "StallTest5.h"
#include "VuAssembler.h"

void CStallTest5::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Sims 2: Castaway, stalling instruction reads Q

	{
		CVuAssembler assembler(microMem);

		//pipeTime = 0 (DIV result at pipeTime 7)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF16, CVuAssembler::FVF_W));

		//pipeTime = 1
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 2
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 3
		assembler.Write(
		    CVuAssembler::Upper::MAX(CVuAssembler::DEST_W, CVuAssembler::VF16, CVuAssembler::VF0, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 4, 5, 6, 7 (MULq has to wait for previous MAX instruction to complete)
		assembler.Write(
		    CVuAssembler::Upper::MULq(CVuAssembler::DEST_XYZW, CVuAssembler::VF16, CVuAssembler::VF16),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	virtualMachine.m_cpu.m_State.nCOP2[16] = uint128{Float::_8, Float::_4, Float::_2, Float::_2};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[16].nV0 == Float::_4);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[16].nV1 == Float::_2);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[16].nV2 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[16].nV3 == Float::_1Half);
}
