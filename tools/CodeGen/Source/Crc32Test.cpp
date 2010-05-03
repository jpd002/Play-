#include "Crc32Test.h"
#include "MemStream.h"

bool		CCrc32Test::m_tableBuilt = false;
uint32		CCrc32Test::m_table[0x100];

CCrc32Test::CCrc32Test(const char* input, uint32 result)
: m_testFunction(NULL)
, m_computeFunction(NULL)
, m_input(input)
, m_inputPtr(0)
, m_result(result)
{

}

CCrc32Test::~CCrc32Test()
{
	delete m_testFunction;
	delete m_computeFunction;
}

void CCrc32Test::Run()
{
	m_inputPtr = 0;

	memset(&m_context, 0, sizeof(m_context));
	m_context.state = STATE_TEST;
	
	while(m_context.state != STATE_DONE)
	{
		CMemoryFunction* function(NULL);

		switch(m_context.state)
		{
		case STATE_TEST:
			function = m_testFunction;
			break;
		case STATE_COMPUTE:
			function = m_computeFunction;
			break;
		}

		assert(function != NULL);

		(*function)(&m_context);
	}

	assert(m_context.currentCrc == m_result);
}

void CCrc32Test::Compile(Jitter::CJitter& jitter)
{
	BuildTable();

	CompileTestFunction(jitter);
	CompileComputeFunction(jitter);
}

void CCrc32Test::CompileTestFunction(Jitter::CJitter& jitter)
{
	if(m_testFunction != NULL) return;

	//b = GetByte()
	//if(b == 0)
	//	done

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRef(this);
		jitter.Call(&CCrc32Test::GetNextByte, 1, true);
		jitter.PullRel(offsetof(CONTEXT, nextByte));

		jitter.PushRel(offsetof(CONTEXT, nextByte));
		jitter.PushCst(0);
		jitter.BeginIf(Jitter::CONDITION_EQ);
		{
			jitter.PushCst(STATE_DONE);
			jitter.PullRel(offsetof(CONTEXT, state));
		}
		jitter.Else();
		{
			jitter.PushCst(STATE_COMPUTE);
			jitter.PullRel(offsetof(CONTEXT, state));
		}
		jitter.EndIf();
	}
	jitter.End();

	m_testFunction = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

void CCrc32Test::CompileComputeFunction(Jitter::CJitter& jitter)
{
	if(m_computeFunction != NULL) return;

	//t   = b ^ crc
	//tv  = GetTable(t)
	//t   = crc >> 8
	//crc = t ^ tv

	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		jitter.PushRef(this);

		jitter.PushRel(offsetof(CONTEXT, nextByte));
		jitter.PushRel(offsetof(CONTEXT, currentCrc));
		jitter.Xor();

		jitter.Call(&CCrc32Test::GetTableValue, 2, true);

		jitter.PushRel(offsetof(CONTEXT, currentCrc));
		jitter.Srl(8);

		jitter.Xor();
		jitter.PullRel(offsetof(CONTEXT, currentCrc));

		jitter.PushCst(STATE_TEST);
		jitter.PullRel(offsetof(CONTEXT, state));
	}
	jitter.End();

	m_computeFunction = new CMemoryFunction(codeStream.GetBuffer(), codeStream.GetSize());
}

uint32 CCrc32Test::GetNextByte(CCrc32Test* context)
{
	return context->m_input[context->m_inputPtr++];
}

uint32 CCrc32Test::GetTableValue(CCrc32Test* context, uint32 index)
{
	return m_table[index & 0xFF];
}

void CCrc32Test::BuildTable()
{
	if(m_tableBuilt) return;

	uint32 polynomial = 0xEDB88320;

	for(uint32 i = 0; i < 256; i++)
	{
		uint32 crc = i;

		for(int j = 8; j > 0; j--)
		{
			if(crc & 1)
				crc = (crc >> 1) ^ polynomial;
			else
				crc >>= 1;
		}

		m_table[i] = crc;
	}

	m_tableBuilt = true;
}
