#include "DivLatencyTest.h"
#include "VuAssembler.h"

void CDivLatencyTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	CVuAssembler assembler(microMem);

	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::DIV(CVuAssembler::VF21, CVuAssembler::X, CVuAssembler::VF20, CVuAssembler::X)	// 2.0 / 1.0
	);

	// This mulq uses the value set on entry, 123
	assembler.Write(
		CVuAssembler::Upper::MULq(CVuAssembler::DEST_X, CVuAssembler::VF1, CVuAssembler::VF20), 
		CVuAssembler::Lower::NOP()
	);

	// This mulq uses the results of the first div due to fdiv pipleline stall
	assembler.Write(
		CVuAssembler::Upper::MULq(CVuAssembler::DEST_X, CVuAssembler::VF2, CVuAssembler::VF20),
		CVuAssembler::Lower::DIV(CVuAssembler::VF22, CVuAssembler::X, CVuAssembler::VF20, CVuAssembler::X)	// 6.0 / 1.0
	);

	assembler.Write(
		CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		CVuAssembler::Lower::NOP()
	);

	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::NOP()
	);

	virtualMachine.m_cpu.m_State.nCOP2[20].nV0 = 0x3F800000;	//VF20 = (1, 1, 1, 1)
	virtualMachine.m_cpu.m_State.nCOP2[20].nV1 = 0x3F800000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV2 = 0x3F800000;
	virtualMachine.m_cpu.m_State.nCOP2[20].nV3 = 0x3F800000;

	virtualMachine.m_cpu.m_State.nCOP2[21].nV0 = 0x40000000;	//VF21 = (2, 2, 2, 2)
	virtualMachine.m_cpu.m_State.nCOP2[21].nV1 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[21].nV2 = 0x40000000;
	virtualMachine.m_cpu.m_State.nCOP2[21].nV3 = 0x40000000;

	virtualMachine.m_cpu.m_State.nCOP2[22].nV0 = 0x40c00000;	//VF22 = (6, 6, 6, 6)
	virtualMachine.m_cpu.m_State.nCOP2[22].nV1 = 0x40c00000;
	virtualMachine.m_cpu.m_State.nCOP2[22].nV2 = 0x40c00000;
	virtualMachine.m_cpu.m_State.nCOP2[22].nV3 = 0x40c00000;

	virtualMachine.m_cpu.m_State.nCOP2Q = 0x42f60000;
	virtualMachine.m_cpu.m_State.pipeQ.counter = 0;
	virtualMachine.m_cpu.m_State.pipeQ.heldValue = 0x42f60000;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV0 == 0x42f60000);	// 123.0
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[2].nV0 == 0x40000000);
}
