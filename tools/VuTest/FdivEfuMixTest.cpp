#include "FdivEfuMixTest.h"
#include "VuAssembler.h"

void CFdivEfuMixTest::Execute(CTestVm& virtualMachine)
{
	//Inspired by Shinobi
	//Starts an EFU operation and then a FDIV operation
	//Uses WAITP a bit later and then reads the Q register right after
	//Since WAITP waits for a huge number of cycles, the FDIV
	//operation should be complete by the time WAITP finishes.

	virtualMachine.Reset();

	{
		auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);
		CVuAssembler assembler(microMem);

		auto branch1Label = assembler.CreateLabel();
		auto branch2Label = assembler.CreateLabel();

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::ERLENG(CVuAssembler::VF27));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::DIV(CVuAssembler::VF15, CVuAssembler::FVF_X, CVuAssembler::VF27, CVuAssembler::FVF_Z));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBNE(CVuAssembler::VI0, CVuAssembler::VI1, branch1Label));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI0, CVuAssembler::VI0, branch2Label));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::WAITP());

		assembler.MarkLabel(branch1Label);

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.MarkLabel(branch2Label);

		//Read Q value into VF1
		assembler.Write(
		    CVuAssembler::Upper::MULq(CVuAssembler::DEST_W, CVuAssembler::VF1, CVuAssembler::VF0),
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	virtualMachine.m_cpu.m_State.nCOP2[15] = {Float::_1, Float::_1, Float::_1, Float::_1};
	virtualMachine.m_cpu.m_State.nCOP2[27] = {Float::_0, Float::_0, Float::_2, Float::_1};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2P == Float::_1Half);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2Q == Float::_1Half);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2[1].nV3 == Float::_1Half);
}
