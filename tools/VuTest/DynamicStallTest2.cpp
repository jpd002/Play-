#include "DynamicStallTest2.h"
#include "VuAssembler.h"

void CDynamicStallTest2::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

	//Inspired by Metal Gear Solid 2
	//No stall occurs in this piece of code, but making sure we don't trigger any dynamic stalling

	{
		CVuAssembler assembler(microMem);
		auto jmpTarget = assembler.CreateLabel();

		//pipeTime = 0 (FMAC (flags) result available in pipeTime 4)
		assembler.Write(
		    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XYZW, CVuAssembler::VF0, CVuAssembler::VF21, CVuAssembler::VF13),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 1
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 2 (write VF27x, result available at pipeTime 6)
		assembler.Write(
		    CVuAssembler::Upper::ADDbc(CVuAssembler::DEST_X, CVuAssembler::VF27, CVuAssembler::VF27, CVuAssembler::VF27, CVuAssembler::BC_Z),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 3 (write VF26x, result available at pipeTime 7)
		assembler.Write(
		    CVuAssembler::Upper::ADDbc(CVuAssembler::DEST_X, CVuAssembler::VF26, CVuAssembler::VF26, CVuAssembler::VF26, CVuAssembler::BC_Z),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 4 (FMAND obtain result from SUB operation at time 0)
		assembler.Write(
		    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XYZW, CVuAssembler::VF0, CVuAssembler::VF21, CVuAssembler::VF13),
		    CVuAssembler::Lower::FMAND(CVuAssembler::VI11, CVuAssembler::VI10));

		//pipeTime = 5
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI1, CVuAssembler::VI0, jmpTarget));

		//pipeTime = 6
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		//pipeTime = 7 (VF26x and VF27x are available, no stall, FMAND result from op at time 3)
		assembler.Write(
		    CVuAssembler::Upper::SUB(CVuAssembler::DEST_XYZ, CVuAssembler::VF0, CVuAssembler::VF27, CVuAssembler::VF26),
		    CVuAssembler::Lower::FMAND(CVuAssembler::VI12, CVuAssembler::VI10));

		//pipeTime = 8
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::IBEQ(CVuAssembler::VI1, CVuAssembler::VI0, jmpTarget));

		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());

		assembler.MarkLabel(jmpTarget);

		//
		assembler.Write(
		    CVuAssembler::Upper::NOP() | CVuAssembler::Upper::E_BIT,
		    CVuAssembler::Lower::NOP());

		//
		assembler.Write(
		    CVuAssembler::Upper::NOP(),
		    CVuAssembler::Lower::NOP());
	}

	auto vuMem = reinterpret_cast<uint128*>(virtualMachine.m_vuMem);

	virtualMachine.m_cpu.m_State.nCOP2[13] = uint128{Float::_4, Float::_4, Float::_4, Float::_4};
	virtualMachine.m_cpu.m_State.nCOP2[21] = uint128{Float::_2, Float::_2, Float::_2, Float::_2};

	virtualMachine.m_cpu.m_State.nCOP2VI[1] = 1;
	virtualMachine.m_cpu.m_State.nCOP2VI[10] = 0xFF; //Check sign and zero

	virtualMachine.ExecuteTest(0);

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[11] == 0xF0); //VF21 - VF13 -> negative result
	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[12] == 0x08); //VF26 -> zero result
}
