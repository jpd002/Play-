#include "FlagsTest2.h"
#include "VuAssembler.h"

void CFlagsTest2::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//From Dragonball Z: Infinite World - MAC pipeline has to be large enough to store 5 values

	CVuAssembler assembler(microMem);

	//pipe = 0		//macTime = 0 + 4 = 4
	assembler.Write(
		CVuAssembler::Upper::OPMULA(CVuAssembler::VF19, CVuAssembler::VF29), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 1		//macTime = 1 + 4 = 5
	assembler.Write(
		CVuAssembler::Upper::OPMSUB(CVuAssembler::VF19, CVuAssembler::VF29, CVuAssembler::VF19), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 2		//macTime = 2 + 4 = 6
	assembler.Write(
		CVuAssembler::Upper::MULAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF4, CVuAssembler::VF20, CVuAssembler::BC_W), 
		CVuAssembler::Lower::NOP()
		);

	//pipe = 3		//macTime = 3 + 4 = 7
	assembler.Write(
		CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF1, CVuAssembler::VF20, CVuAssembler::BC_X), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 4		//macTime = 4 + 4 = 8
	assembler.Write(
		CVuAssembler::Upper::MADDAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF2, CVuAssembler::VF20, CVuAssembler::BC_Y), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 5		//macTime = 5 + 4 = 9, read result from pipe time 1
	assembler.Write(
		CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF25, CVuAssembler::VF3, CVuAssembler::VF20, CVuAssembler::BC_Z), 
		CVuAssembler::Lower::FMAND(CVuAssembler::VI11, CVuAssembler::VI12)
	);

	//pipe = 6
	assembler.Write(
		CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		CVuAssembler::Lower::NOP()
	);

	//pipe = 7
	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::NOP()
	);

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = 0x3F800000;		//VF1x = 1
	virtualMachine.m_cpu.m_State.nCOP2[2].nV1 = 0x3F800000;		//VF2y = 1
	virtualMachine.m_cpu.m_State.nCOP2[3].nV2 = 0x3F800000;		//VF3z = 1
	virtualMachine.m_cpu.m_State.nCOP2[4].nV3 = 0x3F800000;		//VF4w = 1

	virtualMachine.m_cpu.m_State.nCOP2[20].nV0 = 0x40000000;	//VF20 = (2, 2, 2, 2)
	virtualMachine.m_cpu.m_State.nCOP2[20].nV1 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV2 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV3 = 0x40000000;

	virtualMachine.m_cpu.m_State.nCOP2VI[12] = 0xFF;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[19].nV0 == 0x00000000);	//VF19 = cross(VF19, VF29)
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[19].nV1 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[19].nV2 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[19].nV3 == 0x00000000);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[25].nV0 == 0x40000000);	//VF25 = mat4(VF1-4) * VF20
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[25].nV1 == 0x40000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[25].nV2 == 0x40000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[25].nV3 == 0x40000000);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV0 == 0x40000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV1 == 0x40000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV2 == 0x00000000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV3 == 0x40000000);

	//Check if Z flag is set for every component (cross product result)
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[11] == 0xF);
}
