#include <stdio.h>
#include <assert.h>
#include "FlagsTest.h"
#include "FlagsTest2.h"

typedef std::function<CTest* ()> TestFactoryFunction;

static const TestFactoryFunction s_factories[] =
{
	[] () { return new CFlagsTest(); },
	[] () { return new CFlagsTest2(); },
};

int main(int argc, const char** argv)
{
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
