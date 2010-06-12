#include "RandomAluTest.h"
#include "MemStream.h"

#define TEST_NUMBER (23)

CRandomAluTest::CRandomAluTest(bool useConstant)
: m_useConstant(useConstant)
, m_function(NULL)
{

}

CRandomAluTest::~CRandomAluTest()
{
	delete m_function;
}

void CRandomAluTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		if(m_useConstant)
		{
			jitter.PushCst(TEST_NUMBER);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, number));
		}

		jitter.PushCst(4);
		jitter.Div();

		jitter.PushCst(34);
		jitter.Sub();

		jitter.PushCst(0xFFFF00FF);
		jitter.And();

		jitter.PullRel(offsetof(CONTEXT, number));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CRandomAluTest::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number = TEST_NUMBER;
	(*m_function)(&m_context);
	TEST_VERIFY(m_context.number == 0xFFFF00E3);
}
