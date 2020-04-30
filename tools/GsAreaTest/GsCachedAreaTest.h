#pragma once

#include "Test.h"

class CGsCachedAreaTest : public CTest
{
public:
	void Execute() override;

private:
	void CheckEmptyArea();
	void CheckDirtyRect();
	void CheckClearDirtyPages();
	void CheckInvalidate();
};
