#include "TriAceTest.h"
#include "VuAssembler.h"

void CTriAceTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//From Radiata Stories - Needs accurate floating point handling to work

	CVuAssembler assembler(microMem);

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::I_BIT,
	    0x42FECCCD);

	//X
	assembler.Write(
	    CVuAssembler::Upper::MULi(CVuAssembler::DEST_X, CVuAssembler::VF2, CVuAssembler::VF1) | CVuAssembler::Upper::I_BIT,
	    0x42546666);

	assembler.Write(
	    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_X, CVuAssembler::VF3, CVuAssembler::VF2) | CVuAssembler::Upper::I_BIT,
	    0x43D80D3E);

	//Y
	assembler.Write(
	    CVuAssembler::Upper::MULi(CVuAssembler::DEST_Y, CVuAssembler::VF2, CVuAssembler::VF1) | CVuAssembler::Upper::I_BIT,
	    0xC7F079B3);

	assembler.Write(
	    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_Y, CVuAssembler::VF3, CVuAssembler::VF2) | CVuAssembler::Upper::I_BIT,
	    0x43A0DA10);

	//Z
	assembler.Write(
	    CVuAssembler::Upper::MULi(CVuAssembler::DEST_Z, CVuAssembler::VF2, CVuAssembler::VF1) | CVuAssembler::Upper::I_BIT,
	    0x43A02666);

	assembler.Write(
	    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_Z, CVuAssembler::VF3, CVuAssembler::VF2) | CVuAssembler::Upper::I_BIT,
	    0x43546E14);

	//W
	assembler.Write(
	    CVuAssembler::Upper::MULi(CVuAssembler::DEST_W, CVuAssembler::VF2, CVuAssembler::VF1) | CVuAssembler::Upper::I_BIT,
	    0x42F23333);

	assembler.Write(
	    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_W, CVuAssembler::VF3, CVuAssembler::VF2) | CVuAssembler::Upper::I_BIT,
	    0x3DD3DD98);

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = 0xB063B75B;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV1 = 0x8701CE82;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV2 = 0x46FCC888;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV3 = 0x793CC535;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[3].nV0 == 0x42546666);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[3].nV1 == 0xC7F079B3);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[3].nV2 == 0x4B1ED5E7);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[3].nV3 == 0x7D1CA47B);
}
