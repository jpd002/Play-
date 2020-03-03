#include "StallTest.h"
#include "VuAssembler.h"

void CStallTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Rogue Galaxy - Looks like a FMAC stall with an impact, but not really

	CVuAssembler assembler(microMem);

	//pipeTime = 0
	assembler.Write(
	    CVuAssembler::Upper::MULAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF1, CVuAssembler::VF10, CVuAssembler::BC_X),
	    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF9, CVuAssembler::FVF_W));

	//pipeTime = 1
	assembler.Write(
	    CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF2, CVuAssembler::VF10, CVuAssembler::BC_Y),
	    CVuAssembler::Lower::NOP());

	//pipeTime = 2
	assembler.Write(
	    CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF3, CVuAssembler::VF10, CVuAssembler::BC_Z),
	    CVuAssembler::Lower::NOP());

	//pipeTime = 3
	//Writing to VF10xyzw, result will be available at pipeTime 7
	assembler.Write(
	    CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF10, CVuAssembler::VF5, CVuAssembler::VF10, CVuAssembler::BC_W),
	    CVuAssembler::Lower::NOP());

	//pipeTime = 4, 5, 6, 7
	//The DIV here will wait for the result to be written to VF10 (by previous instruction), causing a FMAC stall
	//But it won't disrupt the execution, we will get 3 different values in the 3 MULq instructions below
	assembler.Write(
	    CVuAssembler::Upper::MULAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF1, CVuAssembler::VF11, CVuAssembler::BC_X),
	    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF10, CVuAssembler::FVF_W));

	//pipeTime = 8
	assembler.Write(
	    CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF2, CVuAssembler::VF11, CVuAssembler::BC_Y),
	    CVuAssembler::Lower::NOP());

	//pipeTime = 9
	assembler.Write(
	    CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF3, CVuAssembler::VF11, CVuAssembler::BC_Z),
	    CVuAssembler::Lower::NOP());

	//pipeTime = 10
	assembler.Write(
	    CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF11, CVuAssembler::VF5, CVuAssembler::VF11, CVuAssembler::BC_W),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MULq(CVuAssembler::DEST_XYZ, CVuAssembler::VF12, CVuAssembler::VF9),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MULq(CVuAssembler::DEST_XYZ, CVuAssembler::VF9, CVuAssembler::VF10),
	    CVuAssembler::Lower::DIV(CVuAssembler::VF0, CVuAssembler::FVF_W, CVuAssembler::VF11, CVuAssembler::FVF_W));

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MULq(CVuAssembler::DEST_XYZ, CVuAssembler::VF11, CVuAssembler::VF11),
	    CVuAssembler::Lower::WAITQ());

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	//Some transformation matrix (identity)
	virtualMachine.m_cpu.m_State.nCOP2[1] = uint128{Float::_1, Float::_0, Float::_0, Float::_0};
	virtualMachine.m_cpu.m_State.nCOP2[2] = uint128{Float::_0, Float::_1, Float::_0, Float::_0};
	virtualMachine.m_cpu.m_State.nCOP2[3] = uint128{Float::_0, Float::_0, Float::_1, Float::_0};
	virtualMachine.m_cpu.m_State.nCOP2[5] = uint128{Float::_0, Float::_0, Float::_0, Float::_1};

	//Some vectors that will be normalized
	virtualMachine.m_cpu.m_State.nCOP2[9] = uint128{Float::_2, Float::_2, Float::_2, Float::_2};
	virtualMachine.m_cpu.m_State.nCOP2[10] = uint128{Float::_4, Float::_4, Float::_4, Float::_4};
	virtualMachine.m_cpu.m_State.nCOP2[11] = uint128{Float::_8, Float::_8, Float::_8, Float::_8};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[12].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[12].nV1 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[12].nV2 == Float::_1);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[9].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[9].nV1 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[9].nV2 == Float::_1);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[11].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[11].nV1 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[11].nV2 == Float::_1);
}
