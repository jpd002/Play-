#include "GsCachedAreaTest.h"
#include "GsCachedArea.h"
#include "GSHandler.h"
#include "GsPixelFormats.h"

void CGsCachedAreaTest::Execute()
{
	CheckEmptyArea();
	CheckDirtyRect();
	CheckClearDirtyPages();
	CheckInvalidate();
}

void CGsCachedAreaTest::CheckEmptyArea()
{
	CGsCachedArea area;
	area.SetArea(CGSHandler::PSMCT32, 0, 0, 0);

	auto areaRect = area.GetAreaPageRect();
	assert(areaRect.width == 0);
	assert(areaRect.height == 0);

	auto areaSize = area.GetSize();
	assert(areaSize == 0);

	auto dirtyRect = area.GetDirtyPageRect();
	assert(dirtyRect.x == 0);
	assert(dirtyRect.y == 0);
	assert(dirtyRect.width == 0);
	assert(dirtyRect.height == 0);
}

void CGsCachedAreaTest::CheckDirtyRect()
{
	//Completely dirty
	{
		CGsCachedArea area;
		area.SetArea(CGSHandler::PSMCT32, 0, 512, 512);

		auto areaRect = area.GetAreaPageRect();
		for(uint32 y = 0; y < areaRect.height; y++)
		{
			for(uint32 x = 0; x < areaRect.width; x++)
			{
				uint32 pageIndex = x + (y * areaRect.width);
				area.SetPageDirty(pageIndex);
			}
		}

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == 0);
		assert(dirtyRect.y == 0);
		assert(dirtyRect.width == areaRect.width);
		assert(dirtyRect.height == areaRect.height);
	}

	//One page
	{
		CGsCachedArea area;
		area.SetArea(CGSHandler::PSMCT32, 0, 512, 512);

		auto areaRect = area.GetAreaPageRect();
		uint32 dirtyX = 3;
		uint32 dirtyY = 2;
		area.SetPageDirty(dirtyX + (dirtyY * areaRect.width));

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == dirtyX);
		assert(dirtyRect.y == dirtyY);
		assert(dirtyRect.width == 1);
		assert(dirtyRect.height == 1);
	}

	//Completely clear
	{
		CGsCachedArea area;
		area.SetArea(CGSHandler::PSMCT32, 0, 512, 512);

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == 0);
		assert(dirtyRect.y == 0);
		assert(dirtyRect.width == 0);
		assert(dirtyRect.height == 0);
	}
}

void CGsCachedAreaTest::CheckClearDirtyPages()
{
	CGsCachedArea area;
	area.SetArea(CGSHandler::PSMCT32, 0, 512, 512);

	auto areaRect = area.GetAreaPageRect();
	uint32 dirtyX = 3;
	uint32 dirtyY = 2;
	area.SetPageDirty(dirtyX + (dirtyY * areaRect.width));
	assert(area.HasDirtyPages());

	area.ClearDirtyPages(CGsCachedArea::PageRect { 0, 0, 1, 1 });
	assert(area.HasDirtyPages());

	area.ClearDirtyPages(CGsCachedArea::PageRect { 0, 0, 3, 2 });
	assert(area.HasDirtyPages());

	area.ClearDirtyPages(CGsCachedArea::PageRect { 3, 2, 1, 1 });
	assert(!area.HasDirtyPages());
}

void CGsCachedAreaTest::CheckInvalidate()
{
	auto pixelFormat = CGSHandler::PSMCT32;
	auto pageSize = CGsPixelFormats::GetPsmPageSize(pixelFormat);
	uint32 areaWidth = 512;
	uint32 areaPageWidth = areaWidth / pageSize.first;

	//Invalidate first line
	{
		CGsCachedArea area;
		area.SetArea(pixelFormat, 0, areaWidth, 512);

		area.Invalidate(0, areaPageWidth * CGsPixelFormats::PAGESIZE);

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == 0);
		assert(dirtyRect.y == 0);
		assert(dirtyRect.width == areaPageWidth);
		assert(dirtyRect.height == 1);
	}

	//Invalidate second line
	{
		CGsCachedArea area;
		area.SetArea(pixelFormat, 0, areaWidth, 512);

		area.Invalidate(areaPageWidth * CGsPixelFormats::PAGESIZE, areaPageWidth * CGsPixelFormats::PAGESIZE);

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == 0);
		assert(dirtyRect.y == 1);
		assert(dirtyRect.width == areaPageWidth);
		assert(dirtyRect.height == 1);
	}

	//Invalidate two first lines
	{
		CGsCachedArea area;
		area.SetArea(pixelFormat, 0, areaWidth, 512);

		area.Invalidate(0, areaPageWidth * CGsPixelFormats::PAGESIZE * 2);

		auto dirtyRect = area.GetDirtyPageRect();
		assert(dirtyRect.x == 0);
		assert(dirtyRect.y == 0);
		assert(dirtyRect.width == areaPageWidth);
		assert(dirtyRect.height == 2);
	}
}
