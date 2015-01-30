#pragma once

#include "TestVm.h"

#define TEST_VERIFY(a) if(!(a)) { int* p = 0; (*p) = 0; }

class CTest
{
public:
	virtual			~CTest() {}
	virtual void	Execute(CTestVm&) = 0;
};
