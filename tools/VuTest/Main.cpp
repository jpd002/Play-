#include <stdio.h>
#include <assert.h>
#include "FlagsTest.h"

typedef std::function<CTest* ()> TestFactoryFunction;

static const TestFactoryFunction s_factories[] =
{
	[] () { return new CFlagsTest(); },
};

int main(int argc, const char** argv)
{
	CTestVm virtualMachine;
	
	for(const auto& factory : s_factories)
	{
		auto test = factory();
		test->Execute(virtualMachine);
		delete test;
	}
	return 0;
}
