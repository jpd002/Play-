#include "MultTest.h"
#include "MemStream.h"

CMultTest::CMultTest(bool isSigned)
: m_function(NULL)
, m_isSigned(isSigned)
{

}

CMultTest::~CMultTest()
{

}

void CMultTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = 0xFFFF8000;
	m_context.relArg1 = 0x8000FFFF;

	(*m_function)(&m_context);

	if(!m_isSigned)
	{
		assert(m_context.cstResultLo == 0x30200000);
		assert(m_context.cstResultHi == 0x20205020);

		assert(m_context.relResultLo == 0x80008000);
		assert(m_context.relResultHi == 0x8000BFFE);
	}
	else
	{
		assert(m_context.cstResultLo == 0x30200000);
		assert(m_context.cstResultHi == 0xDFDFD020);
		
		assert(m_context.relResultLo == 0x80008000);
		assert(m_context.relResultHi == 0x00003FFF);
	}
}

void CMultTest::Compile(Jitter::CJitter& jitter)
{
	if(m_function != NULL) return;

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Cst x Cst
		jitter.PushCst(0x80004040);
		jitter.PushCst(0x40408000);

		if(m_isSigned)
		{
			jitter.MultS();
		}
		else
		{
			jitter.Mult();
		}

		jitter.PushTop();

		jitter.ExtLow64();
		jitter.PullRel(offsetof(CONTEXT, cstResultLo));

		jitter.ExtHigh64();
		jitter.PullRel(offsetof(CONTEXT, cstResultHi));

		//Rel x Rel
		jitter.PushRel(offsetof(CONTEXT, relArg0));
		jitter.PushRel(offsetof(CONTEXT, relArg1));

		if(m_isSigned)
		{
			jitter.MultS();
		}
		else
		{
			jitter.Mult();
		}

		jitter.PushTop();

		jitter.ExtLow64();
		jitter.PullRel(offsetof(CONTEXT, relResultLo));

		jitter.ExtHigh64();
		jitter.PullRel(offsetof(CONTEXT, relResultHi));
	}
	jitter.End();

	m_function = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}
