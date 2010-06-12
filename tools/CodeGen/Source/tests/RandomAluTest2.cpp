#include "RandomAluTest2.h"
#include "MemStream.h"

#define TEST_NUMBER1 (0xFF00FF00)
#define TEST_NUMBER2 (0x8888FFFF)

CRandomAluTest2::CRandomAluTest2(bool useConstant)
: m_useConstant(useConstant)
, m_function(NULL)
{

}

CRandomAluTest2::~CRandomAluTest2()
{
	delete m_function;
}

void CRandomAluTest2::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		if(m_useConstant)
		{
			jitter.PushCst(TEST_NUMBER1);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, number1));
		}
		jitter.Not();
		jitter.SignExt8();

		jitter.PushCst(0);
		if(m_useConstant)
		{
			jitter.PushCst(TEST_NUMBER2);
		}
		else
		{
			jitter.PushRel(offsetof(CONTEXT, number2));
		}
		jitter.SignExt16();
		jitter.Sub();

		jitter.Add();

		jitter.PullRel(offsetof(CONTEXT, result));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CRandomAluTest2::Run()
{
	memset(&m_context, 0, sizeof(CONTEXT));
	m_context.number1 = TEST_NUMBER1;
	m_context.number2 = TEST_NUMBER2;
	m_context.result = -1;
	(*m_function)(&m_context);
	TEST_VERIFY(m_context.result == 0);
}
