#include "FpuTest.h"
#include "MemStream.h"

CFpuTest::CFpuTest()
{

}

CFpuTest::~CFpuTest()
{
	delete m_function;
}

void CFpuTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Add();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Div();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Rcpl();
		jitter.FP_PullSingle(offsetof(CONTEXT, number1));

		jitter.FP_PushSingle(offsetof(CONTEXT, number1));
		jitter.FP_Neg();
		jitter.FP_PullSingle(offsetof(CONTEXT, number2));

		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_PushSingle(offsetof(CONTEXT, number2));
		jitter.FP_Mul();
		jitter.FP_PullSingle(offsetof(CONTEXT, number3));

		jitter.FP_PushSingle(offsetof(CONTEXT, number3));
		jitter.FP_Sqrt();
		jitter.FP_PullSingle(offsetof(CONTEXT, number3));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CFpuTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = 1.0;
	m_context.number2 = 2.0;
	(*m_function)(&m_context);
	TEST_VERIFY(m_context.number1 ==  1.5f);
	TEST_VERIFY(m_context.number2 == -1.5f);
	TEST_VERIFY(m_context.number1 == m_context.number3);
}
