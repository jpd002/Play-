#include <functional>
#include "DefaultAppConfig.h"
#include "KeyOnOffTest.h"
#include "MultiCoreIrqTest.h"
#include "SetRepeatTest.h"
#include "SetRepeatTest2.h"
#include "SimpleIrqTest.h"
#include "SweepTest.h"

typedef std::function<CTest*()> TestFactoryFunction;

// clang-format off
static const TestFactoryFunction s_factories[] =
{
	[]() { return new CKeyOnOffTest(); },
	[]() { return new CMultiCoreIrqTest(); },
	[]() { return new CSetRepeatTest(); },
	[]() { return new CSetRepeatTest2(); },
	[]() { return new CSimpleIrqTest(); },
	[]() { return new CSweepTest(); },
};
// clang-format on

int main(int argc, const char** argv)
{
	for(const auto& factory : s_factories)
	{
		auto test = factory();
		test->Execute();
		delete test;
	}
	return 0;
}
