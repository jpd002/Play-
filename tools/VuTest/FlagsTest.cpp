#include "FlagsTest.h"
#include "VuAssembler.h"

void CFlagsTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//From Champions: Return to Arms - MAC op latency needs to be 4 so that FSAND reads the proper result

	CVuAssembler assembler(microMem);

	//pipe = 0		//macTime = 0 + 4 = 4
	assembler.Write(
		CVuAssembler::Upper::SUBbc(CVuAssembler::DEST_W, CVuAssembler::VF0, CVuAssembler::VF2, CVuAssembler::VF1, CVuAssembler::BC_X), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 1
	assembler.Write(
		CVuAssembler::Upper::ITOF0(CVuAssembler::DEST_YW, CVuAssembler::VF3, CVuAssembler::VF21), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 2		//macTime = 2 + 4 = 6
	assembler.Write(
		CVuAssembler::Upper::MULAbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF20, CVuAssembler::VF15, CVuAssembler::BC_W), 
		CVuAssembler::Lower::NOP()
		);

	//pipe = 3		//macTime = 3 + 4 = 7
	assembler.Write(
		CVuAssembler::Upper::MADDbc(CVuAssembler::DEST_XYZW, CVuAssembler::VF17, CVuAssembler::VF17, CVuAssembler::VF15, CVuAssembler::BC_Y), 
		CVuAssembler::Lower::NOP()
	);

	//pipe = 4
	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::NOP()
	);

	//pipe = 5		//check result from SUBbc operation (valid from pipe >= 1)
	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::FSAND(CVuAssembler::VI13, 0xF)
	);

	assembler.Write(
		CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		CVuAssembler::Lower::NOP()
	);

	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::NOP()
	);

	virtualMachine.m_cpu.m_State.nCOP2[1].nV0 = 0;		//VF1x = 0
	virtualMachine.m_cpu.m_State.nCOP2[2].nV3 = 0;		//VF2w = 0

	virtualMachine.m_cpu.m_State.nCOP2[20].nV0 = 0x3F800000;	//VF20 = (1, 1, 1, 1)
	virtualMachine.m_cpu.m_State.nCOP2[20].nV1 = 0x3F800000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV2 = 0x3F800000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV3 = 0x3F800000;

	virtualMachine.m_cpu.m_State.nCOP2[15].nV3 = 0x3F800000;	//VF15w = 1

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV0 == 0x3F800000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV1 == 0x3F800000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV2 == 0x3F800000);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2A.nV3 == 0x3F800000);

	//Check if Z flag is set and S cleared
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[13] == 0x1);
}
