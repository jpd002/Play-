#ifdef WIN32

	#ifdef AMD64
		#include "Jitter_CodeGen_Arm.h"
		#include "Jitter_CodeGen_x86_64.h"
	#else
		#include "Jitter_CodeGen_x86_32.h"
	#endif

#elif defined(__APPLE__)

	#include "TargetConditionals.h"
	#if !(TARGET_IPHONE_SIMULATOR)
		#include "Jitter_CodeGen_Arm.h"
	#else
		#include "Jitter_CodeGen_x86_64.h"
	#endif

#endif

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include "tests/Crc32Test.h"
#include "tests/MultTest.h"
#include "tests/RandomAluTest.h"
#include "tests/RandomAluTest2.h"
#include "tests/FpuTest.h"
#include "tests/CompareTest.h"

typedef boost::function<CTest* ()> TestFactoryFunction;

TestFactoryFunction s_factories[] =
{
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CCompareTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest2>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest2>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CCrc32Test>(), "Hello World!", 0x67FCDACC)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMultTest>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMultTest>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CFpuTest>())),
	TestFactoryFunction(),
};

int main(int argc, char** argv)
{
#ifdef WIN32
	
	#ifdef AMD64
		Jitter::CJitter jitter(new Jitter::CCodeGen_x86_64());
	#else
		Jitter::CJitter jitter(new Jitter::CCodeGen_x86_32());
	#endif
	
#elif defined(__APPLE__)
	
	#if !(TARGET_IPHONE_SIMULATOR)
		Jitter::CJitter jitter(new Jitter::CCodeGen_Arm());
	#else
		Jitter::CJitter jitter(new Jitter::CCodeGen_x86_64());
	#endif

#endif
	
	TestFactoryFunction* currentTestFactory = s_factories;
	while(!currentTestFactory->empty())
	{
		CTest* test = (*currentTestFactory)();
		test->Compile(jitter);
		test->Run();
		delete test;
		currentTestFactory++;
	}
	return 0;
}
