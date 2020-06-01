#include "FlagsTest4.h"
#include "VuAssembler.h"

void CFlagsTest4::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//From Metal Gear Solid 3 - sticky flags need to be properly pipelined

	CVuAssembler assembler(microMem);

	assembler.Write(
	    CVuAssembler::Upper::SUB(CVuAssembler::DEST_W, CVuAssembler::VF0, CVuAssembler::VF0, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::ADDbc(CVuAssembler::DEST_W, CVuAssembler::VF0, CVuAssembler::VF2, CVuAssembler::VF0, CVuAssembler::BC_X),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MULAbc(CVuAssembler::DEST_XYZ, CVuAssembler::VF9, CVuAssembler::VF2, CVuAssembler::BC_W),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZ, CVuAssembler::VF3, CVuAssembler::VF8, CVuAssembler::VF0, CVuAssembler::BC_W),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XYZ, CVuAssembler::VF16, CVuAssembler::VF13, CVuAssembler::VF12),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XYZ, CVuAssembler::VF17, CVuAssembler::VF14, CVuAssembler::VF13),
	    CVuAssembler::Lower::FSAND(CVuAssembler::VI1, 0x80));

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[8].nV0 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[8].nV1 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[8].nV2 = Float::_Minus1;
	virtualMachine.m_cpu.m_State.nCOP2[8].nV3 = Float::_Minus1;

	virtualMachine.ExecuteTest(0);

	//Check that SS (sign sticky) is not set
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[1] == 0);
}
