#include "XgKickTest2.h"
#include "VuAssembler.h"

void CXgKickTest2::Execute(CTestVm& virtualMachine)
{
	static const uint16 constant1 = 0x12;
	static const uint16 constant2 = 0x34;
	static const uint32 packetBaseAddress = 0x100;
	static const uint16 testValue = 0x5555;

	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Dark Cloud (writes to GIF packet after 2 pipeTimes, but no effect since there's a stall on the ISWR instruction)

	CVuAssembler assembler(microMem);

	//pipeTime = 0
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI4, CVuAssembler::VI0, packetBaseAddress / 0x10));

	//pipeTime = 1
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI1, CVuAssembler::VI0, constant1));

	//pipeTime = 2
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI2, CVuAssembler::VI0, constant2));

	//pipeTime = 3 (XGKICK to start in pipeTime 6)
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::XGKICK(CVuAssembler::VI4));

	//pipeTime = 4 (VI2 result will be available to ISW instruction at pipeTime 8)
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADD(CVuAssembler::VI2, CVuAssembler::VI2, CVuAssembler::VI1));

	//pipeTime = 5, 6, 7, 8 (stalls here because using VI2 which has been written to in previous register)
	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::ISWR(CVuAssembler::DEST_X, CVuAssembler::VI2, CVuAssembler::VI4));

	//pipeTime = 9
	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	//We need to make sure this doesn't change after execution
	*reinterpret_cast<uint16*>(virtualMachine.m_vuMem + packetBaseAddress) = testValue;

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_xgKickSnapshots.size() == 1);
	TEST_VERIFY(*reinterpret_cast<uint16*>(virtualMachine.m_xgKickSnapshots[0].data() + packetBaseAddress) == testValue);
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[2] == (constant1 + constant2));
}
