#include "FlagsTest3.h"
#include "VuAssembler.h"

void CFlagsTest3::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Kingdom Hearts 2 - Very simple flags test

	CVuAssembler assembler(microMem);

	for(uint32 i = 0; i < 6; i++)
	{
		assembler.Write(
		    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_XYZW, CVuAssembler::VF0, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());
	}

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::FMAND(CVuAssembler::VI9, CVuAssembler::VI7));

	for(uint32 i = 0; i < 6; i++)
	{
		assembler.Write(
		    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_XYZW, CVuAssembler::VF0, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());
	}

	assembler.Write(
	    CVuAssembler::Upper::OPMULA(CVuAssembler::VF6, CVuAssembler::VF23),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::OPMSUB(CVuAssembler::VF1, CVuAssembler::VF23, CVuAssembler::VF6),
	    CVuAssembler::Lower::NOP());

	for(uint32 i = 0; i < 10; i++)
	{
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::FMAND(CVuAssembler::VI10, CVuAssembler::VI7));

	for(uint32 i = 0; i < 10; i++)
	{
		assembler.Write(
		    CVuAssembler::Upper::ADDi(CVuAssembler::DEST_XYZW, CVuAssembler::VF0, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());
	}

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = 0x40000000; //VF1 = (2, 2, 2, 2)
	virtualMachine.m_cpu.m_State.nCOP2[1].nV1 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV2 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[1].nV3 = 0x40000000;

	virtualMachine.m_cpu.m_State.nCOP2[6].nV0 = 0x3F800000; //VF6 = (1, 0, 0, 1)
	virtualMachine.m_cpu.m_State.nCOP2[6].nV1 = 0x3F800000;
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

	//VF1w should be unchanged from initial value
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV0 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV1 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV2 == 0xBF800000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV3 == 0x40000000);

	//Check sign and zero flags
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[9] == 0xE);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[10] == 0x2C);
}
