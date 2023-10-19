#include "DynamicStallTest.h"
#include "VuAssembler.h"

void CDynamicStallTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Time Crisis 4
	//Code expects two different pipeline timings.
	//One code branch doesn't stall and the other one does.

	{
		CVuAssembler assembler(microMem);
		auto jmpTarget = assembler.CreateLabel();

		//pipeTime = 0 (FMAC result available in pipeTime 4, DIV result will be available in pipeTime 7)
		assembler.Write(
		    CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF10, CVuAssembler::VF8, CVuAssembler::VF0, CVuAssembler::BC_W),
		    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF19, CVuAssembler::FVF_W));

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
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//Run 1 - pipeTime = 4 (operation at pipeTime 0 completed, no stall)
		//Run 2 - pipeTime = 12-13-14 (stalling because FMAC operation is not over)
		assembler.MarkLabel(jmpTarget);
		assembler.Write(
		    CVuAssembler::Upper::CLIP(CVuAssembler::VF10, CVuAssembler::VF10),
		    CVuAssembler::Lower::NOP());

		//Run 1 - pipeTime = 5
		//Run 2 - pipeTime = 15
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//Run 1 - pipeTime = 6
		//Run 2 - pipeTime = 16
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//Run 1 - pipeTime = 7 (DIV result is available)
		//Run 2 - pipeTime = 17 (DIV result is available)
		assembler.Write(
		    CVuAssembler::Upper::MULq(CVuAssembler::DEST_W, CVuAssembler::VF21, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());

		//Run 1 - pipeTime = 8
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IADDIU(CVuAssembler::VI1, CVuAssembler::VI1, 1));

		//Run 1 - pipeTime = 9 (DIV result will be available at pipeTime 16)
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF20, CVuAssembler::FVF_W));

		//Run 1 - pipeTime = 10 (FMAC result will be available at pipeTime 14)
		assembler.Write(
		    CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF10, CVuAssembler::VF8, CVuAssembler::VF0, CVuAssembler::BC_W),
		    CVuAssembler::Lower::IBNE(CVuAssembler::VI1, CVuAssembler::VI2, jmpTarget));

		//Run 1 - pipeTime = 11
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::SQ(CVuAssembler::DEST_XYZW, CVuAssembler::VF21, 0xFFFF, CVuAssembler::VI1));

		//
		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		//
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	auto vuMem = reinterpret_cast<uint128*>(virtualMachine.m_vuMem);

	virtualMachine.m_cpu.m_State.nCOP2[19] = uint128{Float::_2, Float::_2, Float::_2, Float::_2};
	virtualMachine.m_cpu.m_State.nCOP2[20] = uint128{Float::_4, Float::_4, Float::_4, Float::_4};

	virtualMachine.m_cpu.m_State.nCOP2VI[1] = 0;
	virtualMachine.m_cpu.m_State.nCOP2VI[2] = 2;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[2] == 2);
	TEST_VERIFY(vuMem[0].nV3 == Float::_1Half);
	TEST_VERIFY(vuMem[1].nV3 == Float::_1Quarter);
}
