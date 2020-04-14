#include "MinMaxTest.h"
#include "VuAssembler.h"

void CMinMaxTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Dynasty Warriors 5
	//Will use MINI on a vector, W contains alpha value as byte
	//Which is a denormal if we interpret is as a float
	//The denormal number should not be flushed to zero
	//DW5 also requires that MAX preserve denormals

	CVuAssembler assembler(microMem);

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_XYZW, CVuAssembler::VF4, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MAX(CVuAssembler::DEST_XYZW, CVuAssembler::VF5, CVuAssembler::VF1, CVuAssembler::VF3),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = Float::_1;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV1 = Float::_Minus8;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV2 = Float::_64;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV3 = 0x80;

	virtualMachine.m_cpu.m_State.nCOP2[2].nV0 = Float::_8;
	virtualMachine.m_cpu.m_State.nCOP2[2].nV1 = Float::_8;
	virtualMachine.m_cpu.m_State.nCOP2[2].nV2 = Float::_8;
	virtualMachine.m_cpu.m_State.nCOP2[2].nV3 = Float::_8;

	virtualMachine.m_cpu.m_State.nCOP2[3].nV0 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[3].nV1 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[3].nV2 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[3].nV3 = Float::_Minus1;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[4].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[4].nV1 == Float::_Minus8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[4].nV2 == Float::_8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[4].nV3 == 0x80);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[5].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[5].nV1 == Float::_Minus1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[5].nV2 == Float::_64);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[5].nV3 == 0x80);
}
