#include "StallTest4.h"
#include "VuAssembler.h"

void CStallTest4::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by God of War, similar to stall test 3, but with added branches

	{
		CVuAssembler assembler(microMem);

		auto branchLabel = assembler.CreateLabel();

		//pipeTime = 0, writing to VF19xy, result will be available in pipeTime 4
		assembler.Write(
		    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XY, CVuAssembler::VF19, CVuAssembler::VF1, CVuAssembler::VF23),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 1, CLIP result will be available in pipeTime 5
		assembler.Write(
		    CVuAssembler::Upper::CLIP(CVuAssembler::VF2, CVuAssembler::VF0),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI15, CVuAssembler::VI0, branchLabel));

		//pipeTime = 2, CLIP result will be available in pipeTime 6
		assembler.Write(
		    CVuAssembler::Upper::CLIP(CVuAssembler::VF15, CVuAssembler::VF0),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI3, CVuAssembler::VI0, 0x3F));

		//pipeTime = 3, 4, reading VF19x, stalling till pipeTime 4
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::MTIR(CVuAssembler::VI10, CVuAssembler::VF19, CVuAssembler::FVF_X));

		//pipeTime = 5
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::B(branchLabel));

		//pipeTime = 6
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::FCGET(CVuAssembler::VI10));

		assembler.MarkLabel(branchLabel);

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	virtualMachine.m_cpu.m_State.nCOP2VI[15] = 0xFFFF;

	virtualMachine.m_cpu.m_State.nCOP2[2] = uint128{Float::_2, Float::_Minus1, Float::_2, Float::_1};
	virtualMachine.m_cpu.m_State.nCOP2[15] = uint128{Float::_Minus1, Float::_2, Float::_Minus1, Float::_1};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[10] == 0x444);
}
