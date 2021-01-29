#include "StallTest3.h"
#include "VuAssembler.h"

void CStallTest3::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by God of War

	CVuAssembler assembler(microMem);

	//pipeTime = 0, CLIP & MFIR results available in pipeTime 4
	assembler.Write(
	    CVuAssembler::Upper::CLIP(CVuAssembler::VF18, CVuAssembler::VF0),
	    CVuAssembler::Lower::MFIR(CVuAssembler::DEST_X, CVuAssembler::VF1, CVuAssembler::VI10));

	//pipeTime = 1, CLIP result available in pipeTime 5
	assembler.Write(
	    CVuAssembler::Upper::CLIP(CVuAssembler::VF19, CVuAssembler::VF0),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI15, CVuAssembler::VI0, 0xBEF));

	//pipeTime = 2, 3, 4
	//FMAC stalling till pipeTime 4 because results in VF1x are not ready
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::MTIR(CVuAssembler::VI10, CVuAssembler::VF1, CVuAssembler::FVF_X));

	//pipeTime = 5
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::FCGET(CVuAssembler::VI10));

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.m_cpu.m_State.nCOP2[18] = uint128{Float::_2, Float::_Minus1, Float::_2, Float::_1};
	virtualMachine.m_cpu.m_State.nCOP2[19] = uint128{Float::_Minus1, Float::_2, Float::_Minus1, Float::_1};

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[10] == 0x444);
}
