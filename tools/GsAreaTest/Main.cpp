#include <functional>
#include "GsCachedAreaTest.h"
#include "GsSpriteRegionTest.h"
#include "GsTransferInvalidationTest.h"

typedef std::function<CTest*()> TestFactoryFunction;

// clang-format off
static const TestFactoryFunction s_factories[] =
{
	[]() { return new CGsCachedAreaTest(); },
	[]() { return new CGsSpriteRegionTest(); },
	[]() { return new CGsTransferInvalidationTest(); }
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
