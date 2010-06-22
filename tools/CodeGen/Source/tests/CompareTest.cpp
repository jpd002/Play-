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

	(*m_function)(&m_context);
}

void CCompareTest::Compile(Jitter::CJitter& jitter)
{
	if(m_function != NULL) return;

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRel(offsetof(CONTEXT, number1));
		jitter.PushRel(offsetof(CONTEXT, number2));
		jitter.Sra();
		jitter.PullRel(offsetof(CONTEXT, number1));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
