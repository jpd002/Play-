#include "MinMaxFlagsTest.h"
#include "VuAssembler.h"

void CMinMaxFlagsTest::Execute(CTestVm& virtualMachine)
{
	auto flagsMaskRegister = CVuAssembler::VI1;
	auto flagsResultRegister = CVuAssembler::VI2;

	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	CVuAssembler assembler(microMem);

	//Inspired by Legacy of Kain: Defiance
	//Just make sure that the compiler doesn't think that MINI/MAX changes MACflags
	//It can throw off the flags computation elision optimization and give wrong results

	//This will set the MACflag to ZX = 1
	assembler.Write(
	    CVuAssembler::Upper::ADDbc(CVuAssembler::DEST_X, CVuAssembler::VF0, CVuAssembler::VF0, CVuAssembler::VF0, CVuAssembler::BC_Z),
	    CVuAssembler::Lower::IADDIU(flagsMaskRegister, CVuAssembler::VI0, 0x7FFF));

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::MINI(CVuAssembler::DEST_YZW, CVuAssembler::VF3, CVuAssembler::VF1, CVuAssembler::VF2),
	    CVuAssembler::Lower::FMAND(flagsResultRegister, flagsMaskRegister));

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[flagsResultRegister] == 0x8);
}
