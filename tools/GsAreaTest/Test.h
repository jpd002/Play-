#pragma once

#define TEST_VERIFY(a) \
	if(!(a))           \
	{                  \
		int* p = 0;    \
		(*p) = 0;      \
	}

class CTest
{
public:
	virtual ~CTest() = default;
	virtual void Execute() = 0;
};
