#ifndef _RANDOMALUTEST2_H_
#define _RANDOMALUTEST2_H_

#include "Test.h"
#include "../MemoryFunction.h"

class CRandomAluTest2 : public CTest
{
public:
						CRandomAluTest2(bool);
	virtual				~CRandomAluTest2();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct CONTEXT
	{
		uint32 number1;
		uint32 number2;
		uint32 result;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
