#include "FlagsTest.h"

void CFlagsTest::Execute(CTestVm& virtualMachine)
{
	virtualMachine.Reset();

	auto microMem = reinterpret_cast<uint32*>(virtualMachine.m_microMem);

/*
00001F78    00211004 01F35802    SUB            VF0w, VF2w, VF1x               LQ              VF19xyzw, $0002(VI11)		//pipe = 0		//macTime = 0 + 4 = 4
00001F80    00A3A93C 01FE5803    ITOF0          VF3yw, VF21yw                  LQ              VF30xyzw, $0003(VI11)		//pipe = 1
00001F88    01EFA1BF 01F45003    MULA           ACCxyzw, VF20xyzw, VF15w       LQ              VF20xyzw, $0003(VI10)		//pipe = 2		//macTime = 2 + 4 = 6
00001F90    01EF8C49 01F55001    MADD           VF17xyzw, VF17xyzw, VF15y      LQ              VF21xyzw, $0001(VI10)		//pipe = 3
00001F98    000002FF 01FF5002    NOP                                           LQ              VF31xyzw, $0002(VI10)		//pipe = 4
00001FA0    000002FF 2C0D0001    NOP                                           FSAND           VI13, 0x001					//pipe = 5
*/

	microMem[1] = 0x00211004;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x00A3A93C;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x01EFA1BF;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x01EF8C49;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x000002FF;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x000002FF;		microMem[0] = 0x2C0D0001;		microMem += 2;
	microMem[1] = 0x400002FF;		microMem[0] = 0x8000033C;		microMem += 2;
	microMem[1] = 0x000002FF;		microMem[0] = 0x8000033C;		microMem += 2;

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

	TEST_VERIFY(virtualMachine.m_cpu.m_State.nCOP2VI[13] == 0x1);
}
