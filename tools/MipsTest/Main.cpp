#include <stdio.h>
#include <assert.h>
#include "Test.h"
#include "Add64Test.h"
#include "SetLessThanTest.h"
#include "SplitLoadTest.h"
#include "Shift32Test.h"
#include "Shift64Test.h"
#include "ExchangeTest.h"
#include "ExtendTest.h"
#include "PackTest.h"

typedef std::function<CTest* ()> TestFactoryFunction;

static const TestFactoryFunction s_factories[] =
{
	[] () -> CTest* { return new CAdd64Test();			},
	[] () -> CTest* { return new CSetLessThanTest();	},
	[] () -> CTest* { return new CSplitLoadTest();		},
	[] () -> CTest* { return new CShift32Test();		},
	[] () -> CTest* { return new CShift64Test();		},
	[] () -> CTest* { return new CExchangeTest();		},
	[] () -> CTest* { return new CExtendTest();			},
	[]() -> CTest* { return new CPackTest();			}
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
