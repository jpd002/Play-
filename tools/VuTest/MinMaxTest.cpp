#include "MinMaxTest.h"
#include "VuAssembler.h"

void CMinMaxTest::Execute(CTestVm& virtualMachine)
{
	auto resultReg1 = CVuAssembler::VF16;
	auto resultReg2 = CVuAssembler::VF17;
	auto resultReg3 = CVuAssembler::VF18;
	auto resultReg4 = CVuAssembler::VF19;

	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	CVuAssembler assembler(microMem);

	//Inspired by Dynasty Warriors 5
	//Will use MINI on a vector, W contains alpha value as byte
	//Which is a denormal if we interpret is as a float
	//The denormal number should not be flushed to zero
	//DW5 also requires that MAX preserve denormals

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_XYZW, resultReg1, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MAX(CVuAssembler::DEST_XYZW, resultReg2, CVuAssembler::VF1, CVuAssembler::VF3),
	    CVuAssembler::Lower::NOP());

	//Inspired by Gran Turismo 4
	//MINI/MAX needs to work properly with PS2's minimum & maximum float values

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_XYZW, resultReg3, CVuAssembler::VF1, CVuAssembler::VF4),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MAX(CVuAssembler::DEST_XYZW, resultReg4, CVuAssembler::VF5, CVuAssembler::VF1),
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

	virtualMachine.m_cpu.m_State.nCOP2[4].nV0 = Float::_Max;
	virtualMachine.m_cpu.m_State.nCOP2[4].nV1 = Float::_Max;
	virtualMachine.m_cpu.m_State.nCOP2[4].nV2 = Float::_Max;
	virtualMachine.m_cpu.m_State.nCOP2[4].nV3 = Float::_Max;

	virtualMachine.m_cpu.m_State.nCOP2[5].nV0 = Float::_Min;
	virtualMachine.m_cpu.m_State.nCOP2[5].nV1 = Float::_Min;
	virtualMachine.m_cpu.m_State.nCOP2[5].nV2 = Float::_Min;
	virtualMachine.m_cpu.m_State.nCOP2[5].nV3 = Float::_Min;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg1].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg1].nV1 == Float::_Minus8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg1].nV2 == Float::_8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg1].nV3 == 0x80);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg2].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg2].nV1 == Float::_Minus1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg2].nV2 == Float::_64);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg2].nV3 == 0x80);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg3].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg3].nV1 == Float::_Minus8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg3].nV2 == Float::_64);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg3].nV3 == 0x80);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg4].nV0 == Float::_1);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg4].nV1 == Float::_Minus8);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg4].nV2 == Float::_64);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[resultReg4].nV3 == 0x80);
}
