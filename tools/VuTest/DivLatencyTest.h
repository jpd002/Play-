#pragma once

#include "Test.h"

class CDivLatencyTest : public CTest
{
public:
	void	Execute(CTestVm&) override;

private:
	void	TestFdivFdivStall(CTestVm&);
	void	TestNoStall(CTestVm&);
};
