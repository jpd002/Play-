#ifndef _RANDOMALUTEST_H_
#define _RANDOMALUTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CRandomAluTest : public CTest
{
public:
						CRandomAluTest(bool);
	virtual				~CRandomAluTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		uint32 number;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
