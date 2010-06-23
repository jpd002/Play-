#include "CompareTest.h"
#include "MemStream.h"

CCompareTest::CCompareTest()
: m_function(NULL)
{

}

CCompareTest::~CCompareTest()
{

}

void CCompareTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));
	m_context.number1 = 0x80000000;
	m_context.number2 = 0x10;
	m_context.number3 = 0x10000;

	(*m_function)(&m_context);

	TEST_VERIFY(m_context.number1 == 0x10000);
	TEST_VERIFY(m_context.number3 == 0);
}

void CCompareTest::Compile(Jitter::CJitter& jitter)
{
	if(m_function != NULL) return;

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//number1 = ((number1 >> number2) < -1) << number2

		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PushRel(offsetof(CONTEXT, number2));
		jitter.Sra();

		jitter.PushCst(0xFFFFFFFF);
		jitter.Cmp(Jitter::CONDITION_LT);

		//jitter.PushTop();
		//jitter.PullRel(offsetof(CONTEXT, number4));

		jitter.PushRel(offsetof(CONTEXT, number2));
		jitter.Shl();
		jitter.PullRel(offsetof(CONTEXT, number1));

		//number3 = number1 != number3

		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PushRel(offsetof(CONTEXT, number3));
		jitter.Cmp(Jitter::CONDITION_NE);

		jitter.PullRel(offsetof(CONTEXT, number3));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
