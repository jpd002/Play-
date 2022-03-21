#include "XgKickTest.h"
#include "VuAssembler.h"

void CXgKickTest::Execute(CTestVm& virtualMachine)
{
	static const uint32 packetBaseAddress = 0x100;
	static const uint16 testValue = 0x5555;

	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Jaws Unleashed (writes to GIF packet after 2 pipeTimes)

	CVuAssembler assembler(microMem);

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI1, CVuAssembler::VI0, testValue));

	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::MFIR(CVuAssembler::DEST_X, CVuAssembler::VF4, CVuAssembler::VI1));

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::IADDIU(CVuAssembler::VI4, CVuAssembler::VI0, packetBaseAddress / 0x10));

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::XGKICK(CVuAssembler::VI4));

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	assembler.Write(
		CVuAssembler::Upper::NOP(),
		CVuAssembler::Lower::SQ(CVuAssembler::DEST_XYZW, CVuAssembler::VF4, 0, CVuAssembler::VI4));

	assembler.Write(
	    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
	    CVuAssembler::Lower::NOP());

	assembler.Write(
	    CVuAssembler::Upper::NOP(),
	    CVuAssembler::Lower::NOP());

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_xgKickSnapshots.size() == 1);
	TEST_VERIFY(*reinterpret_cast<uint16*>(virtualMachine.m_xgKickSnapshots[0].data() + packetBaseAddress) == testValue);
}
