#pragma once

#include "Test.h"

class CGsSpriteRegionTest : public CTest
{
public:
	void Execute() override;

private:
	void CheckIntersect();
	void CheckReverseCoords();
};
