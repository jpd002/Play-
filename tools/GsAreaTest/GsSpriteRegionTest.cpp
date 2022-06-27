#include "GsSpriteRegionTest.h"
#include "gs/GsSpriteRegion.h"

void CGsSpriteRegionTest::Execute()
{
	CheckIntersect();
	CheckReverseCoords();
}

void CGsSpriteRegionTest::CheckIntersect()
{
	CGsSpriteRegion region;

	{
		CGsSpriteRect empty(0, 0, 0, 0);
		TEST_VERIFY(!region.Intersects(empty));
	}

	region.Insert(CGsSpriteRect(8, 8, 16, 16));

	{
		CGsSpriteRect square(16, 8, 24, 16);
		TEST_VERIFY(!region.Intersects(square));
		region.Insert(square);
		TEST_VERIFY(region.Intersects(square));
	}
}

void CGsSpriteRegionTest::CheckReverseCoords()
{
	CGsSpriteRect square(24, 8, 16, 16);
	TEST_VERIFY(square.GetWidth() == 8);
	TEST_VERIFY(square.GetHeight() == 8);
}
