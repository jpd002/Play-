#include "StallTest2.h"
#include "VuAssembler.h"

void CStallTest2::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Crimson Tears - FMAC stall has an impact on clip result availability

	CVuAssembler assembler(microMem);

	//pipeTime = 0
	//Writing to VF20xyzw here, results will be available at pipeTime 4
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::LQ(CVuAssembler::DEST_XYZW, CVuAssembler::VF20, 0xE, CVuAssembler::VI6));

	//pipeTime = 1
	//CLIP result will be available at pipeTime 5
	assembler.Write(
	    CVuAssembler::Upper::CLIP(CVuAssembler::VF25, CVuAssembler::VF0),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI6, CVuAssembler::VI6, 0x01));

	//pipeTime = 2
	assembler.Write(
	    CVuAssembler::Upper::FTOI4(CVuAssembler::DEST_XYZW, CVuAssembler::VF15, CVuAssembler::VF15),
	    CVuAssembler::Lower::LQ(CVuAssembler::DEST_Y, CVuAssembler::VF12, 0x0D, CVuAssembler::VI0));

	//pipeTime = 3, 4
	//FMAC stalling till pipeTime 4 because results in VF20xyzw are not ready
	assembler.Write(
	    CVuAssembler::Upper::ITOF12(CVuAssembler::DEST_XYZW, CVuAssembler::VF20, CVuAssembler::VF20),
	    CVuAssembler::Lower::SQ(CVuAssembler::DEST_XYZW, CVuAssembler::VF24, 0x01, CVuAssembler::VI10));

	//pipeTime = 5
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::FCAND(0x3F));

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[25] = uint128{Float::_2, Float::_2, Float::_2, Float::_1};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[1] == 1);
}
