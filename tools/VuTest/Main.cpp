#include <stdio.h>
#include <assert.h>
#include <fenv.h>
#include "AddTest.h"
#include "FlagsTest.h"
#include "FlagsTest2.h"
#include "TriAceTest.h"

typedef std::function<CTest*()> TestFactoryFunction;

static const TestFactoryFunction s_factories[] =
    {
        []() { return new CAddTest(); },
        []() { return new CFlagsTest(); },
        []() { return new CFlagsTest2(); },
        []() { return new CTriAceTest(); },
};

int main(int argc, const char** argv)
{
	fesetround(FE_TOWARDZERO);

	CTestVm virtualMachine;

	for(const auto& factory : s_factories)
	{
		virtualMachine.Reset();
		auto test = factory();
		test->Execute(virtualMachine);
		delete test;
	}
	return 0;
}
