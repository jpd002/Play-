#include <stdio.h>
#include <assert.h>
#include <fenv.h>
#include "FpUtils.h"
#include "AddTest.h"
#include "BranchTest.h"
#include "FdivEfuMixTest.h"
#include "FlagsTest.h"
#include "FlagsTest2.h"
#include "FlagsTest3.h"
#include "FlagsTest4.h"
#include "IntBranchDelayTest.h"
#include "IntBranchDelayTest2.h"
#include "MinMaxTest.h"
#include "MinMaxFlagsTest.h"
#include "StallTest.h"
#include "StallTest2.h"
#include "StallTest3.h"
#include "StallTest4.h"
#include "StallTest5.h"
#include "StallTest6.h"
#include "TriAceTest.h"

typedef std::function<CTest*()> TestFactoryFunction;

// clang-format off
static const TestFactoryFunction s_factories[] =
{
	[]() { return new CAddTest(); },
	[]() { return new CBranchTest(); },
	[]() { return new CFdivEfuMixTest(); },
	[]() { return new CFlagsTest(); },
	[]() { return new CFlagsTest2(); },
	[]() { return new CFlagsTest3(); },
	[]() { return new CFlagsTest4(); },
	[]() { return new CIntBranchDelayTest(); },
	[]() { return new CIntBranchDelayTest2(); },
	[]() { return new CMinMaxTest(); },
	[]() { return new CMinMaxFlagsTest(); },
	[]() { return new CStallTest(); },
	[]() { return new CStallTest2(); },
	[]() { return new CStallTest3(); },
	[]() { return new CStallTest4(); },
	[]() { return new CStallTest5(); },
	[]() { return new CStallTest6(); },
	[]() { return new CTriAceTest(); },
};
// clang-format on

int main(int argc, const char** argv)
{
	fesetround(FE_TOWARDZERO);
	FpUtils::SetDenormalHandlingMode();

	auto virtualMachine = std::make_unique<CTestVm>();

	for(const auto& factory : s_factories)
	{
		virtualMachine->Reset();
		auto test = factory();
		test->Execute(*virtualMachine);
		delete test;
	}
	return 0;
}
