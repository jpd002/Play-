#pragma once

#include "TestVm.h"

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
	virtual void Execute(CTestVm&) = 0;

	static uint32 MakeFloat(float input)
	{
		return *reinterpret_cast<uint32*>(&input);
	}

	static uint128 MakeVector(float x, float y, float z, float w)
	{
		uint128 vector =
			{
				MakeFloat(x),
				MakeFloat(y),
				MakeFloat(z),
				MakeFloat(w)
			};
		return vector;
	}
};
