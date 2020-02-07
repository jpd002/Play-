#include "FlagsTest3.h"
#include "VuAssembler.h"

void CFlagsTest3::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//From Kingdom Hearts 2 - Very simple flags test

	CVuAssembler assembler(microMem);

	//pipe = 0
	assembler.Write(
	    CVuAssembler::Upper::OPMULA(CVuAssembler::VF6, CVuAssembler::VF23),
	    CVuAssembler::Lower::NOP());

	//pipe = 1
	assembler.Write(
	    CVuAssembler::Upper::OPMSUB(CVuAssembler::VF0, CVuAssembler::VF23, CVuAssembler::VF6),
	    CVuAssembler::Lower::NOP());

	//pipe = 2
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	//pipe = 3
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	//pipe = 4
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	//pipe = 5
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::FMAND(CVuAssembler::VI10, CVuAssembler::VI7));

	//pipe = 
	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	//pipe = 
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[6].nV0 = 0x3F800000; //VF6 = (1, 0, 0, 1)
	virtualMachine.m_cpu.m_State.nCOP2[6].nV1 = 0x00000000;
	virtualMachine.m_cpu.m_State.nCOP2[6].nV2 = 0x00000000;
	virtualMachine.m_cpu.m_State.nCOP2[6].nV3 = 0x3F800000;

	virtualMachine.m_cpu.m_State.nCOP2[23].nV0 = 0x00000000; //VF23 = (0, -1, 0, 1)
	virtualMachine.m_cpu.m_State.nCOP2[23].nV1 = 0xBF800000;
	virtualMachine.m_cpu.m_State.nCOP2[23].nV2 = 0x00000000;
	virtualMachine.m_cpu.m_State.nCOP2[23].nV3 = 0x3F800000;

	virtualMachine.m_cpu.m_State.nCOP2VI[7] = 0xFFFF;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV0 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV1 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV2 == 0xBF800000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV3 == 0x00000000);

	//Check sign and zero flags
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[10] == 0x2D);
}
