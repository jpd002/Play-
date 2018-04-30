#include "AddTest.h"
#include "VuAssembler.h"

static uint32 FloatToInt(float value)
{
	return *reinterpret_cast<uint32*>(&value);
}

void CAddTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	CVuAssembler assembler(microMem);

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::I_BIT,
	    FloatToInt(2048));

	assembler.Write(
	    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_XYZW, CVuAssembler::VF2, CVuAssembler::VF1),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = FloatToInt(-128);
	virtualMachine.m_cpu.m_State.nCOP2[1].nV1 = FloatToInt(-96);
	virtualMachine.m_cpu.m_State.nCOP2[1].nV2 = FloatToInt(96);
	virtualMachine.m_cpu.m_State.nCOP2[1].nV3 = FloatToInt(128);

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[2].nV0 == FloatToInt(-128 + 2048));
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[2].nV1 == FloatToInt(-96 + 2048));
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[2].nV2 == FloatToInt(96 + 2048));
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[2].nV3 == FloatToInt(128 + 2048));
}
