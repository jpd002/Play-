#include <cassert>
#include "GsCachedArea.h"
#include "GsPixelFormats.h"

static bool DoMemoryRangesOverlap(uint32 start1, uint32 size1, uint32 start2, uint32 size2)
{
	uint32 min1 = start1;
	uint32 max1 = start1 + size1;

	uint32 min2 = start2;
	uint32 max2 = start2 + size2;

	if(max1 <= min2) return false;
	if(min1 >= max2) return false;
	
	return true;
}

CGsCachedArea::CGsCachedArea()
{
	ClearDirtyPages();
}

void CGsCachedArea::SetArea(uint32 psm, uint32 bufPtr, uint32 bufWidth, uint32 height)
{
	m_psm = psm;
	m_bufPtr = bufPtr;
	m_bufWidth = bufWidth;
	m_height = height;
}

CGsCachedArea::PageRect CGsCachedArea::GetAreaPageRect() const
{
	auto texturePageSize = CGsPixelFormats::GetPsmPageSize(m_psm);
	uint32 pageCountX = (m_bufWidth + texturePageSize.first - 1) / texturePageSize.first;
	uint32 pageCountY = (m_height + texturePageSize.second - 1) / texturePageSize.second;
	return PageRect { 0, 0, pageCountX, pageCountY };
}

CGsCachedArea::PageRect CGsCachedArea::GetDirtyPageRect() const
{
	auto areaRect = GetAreaPageRect();

	//Find starting point
	uint32 startX = 0, startY = 0;
	for(startY = 0; startY < areaRect.height; startY++)
	{
		bool done = false;
		for(startX = 0; startX < areaRect.width; startX++)
		{
			uint32 pageIndex = startX + (startY * areaRect.width);
			if(IsPageDirty(pageIndex))
			{
				done = true;
				break;
			}
		}
		if(done) break;
	}

	if((startX == areaRect.width) || (startY == areaRect.height))
	{
		return PageRect { 0, 0, 0, 0 };
	}

	const auto getHorzSpan =
		[&] (uint32 bx, uint32 by)
		{
			uint32 span = 0;
			for(uint32 x = bx; x < areaRect.width; x++)
			{
				uint32 pageIndex = x + (by * areaRect.width);
				if(!IsPageDirty(pageIndex)) break;
				span++;
			}
			return span;
		};

	//Check how high is the dirty rect
	uint32 spanX = getHorzSpan(startX, startY);
	uint32 spanY = 1;
	for(uint32 y = startY + 1; y < areaRect.height; y++)
	{
		uint32 lineSpanX = getHorzSpan(startX, y);
		if(lineSpanX < spanX) break;
		spanY++;
	}

	return PageRect { startX, startY, spanX, spanY };
}

uint32 CGsCachedArea::GetPageCount() const
{
	auto areaRect = GetAreaPageRect();
	return areaRect.width * areaRect.height;
}

uint32 CGsCachedArea::GetSize() const
{
	return GetPageCount() * CGsPixelFormats::PAGESIZE;
}

void CGsCachedArea::Invalidate(uint32 memoryStart, uint32 memorySize)
{
	uint32 areaSize = GetSize();

	if(DoMemoryRangesOverlap(memoryStart, memorySize, m_bufPtr, areaSize))
	{
		//Find the pages that are touched by this transfer
		uint32 pageStart = (memoryStart < m_bufPtr) ? 0 : ((memoryStart - m_bufPtr) / CGsPixelFormats::PAGESIZE);
		uint32 pageCount = (memorySize + CGsPixelFormats::PAGESIZE - 1) / CGsPixelFormats::PAGESIZE;
		uint32 areaPageCount = GetPageCount();

		for(unsigned int i = 0; i < pageCount; i++)
		{
			uint32 pageIndex = pageStart + i;
			if(pageIndex >= areaPageCount) break;
			SetPageDirty(pageIndex);
		}

		//Wouldn't make sense to go through here and not have at least a dirty page
		assert(HasDirtyPages());
	}
}

bool CGsCachedArea::IsPageDirty(uint32 pageIndex) const
{
	assert(pageIndex < sizeof(m_dirtyPages) * 8);
	unsigned int dirtyPageSection = pageIndex / (sizeof(m_dirtyPages[0]) * 8);
	unsigned int dirtyPageIndex = pageIndex % (sizeof(m_dirtyPages[0]) * 8);
	return (m_dirtyPages[dirtyPageSection] & (1ULL << dirtyPageIndex)) != 0;
}

void CGsCachedArea::SetPageDirty(uint32 pageIndex)
{
	assert(pageIndex < sizeof(m_dirtyPages) * 8);
	unsigned int dirtyPageSection = pageIndex / (sizeof(m_dirtyPages[0]) * 8);
	unsigned int dirtyPageIndex = pageIndex % (sizeof(m_dirtyPages[0]) * 8);
	m_dirtyPages[dirtyPageSection] |= (1ULL << dirtyPageIndex);
}

bool CGsCachedArea::HasDirtyPages() const
{
	DirtyPageHolder dirtyStatus = 0;
	for(unsigned int i = 0; i < MAX_DIRTYPAGES_SECTIONS; i++)
	{
		dirtyStatus |= m_dirtyPages[i];
	}
	return (dirtyStatus != 0);
}

void CGsCachedArea::ClearDirtyPages()
{
	memset(m_dirtyPages, 0, sizeof(m_dirtyPages));
}

void CGsCachedArea::ClearDirtyPages(const PageRect& rect)
{
	auto areaRect = GetAreaPageRect();
	uint32 endX = rect.x + rect.width;
	uint32 endY = rect.y + rect.height;

	for(uint32 y = rect.y; y < endY; y++)
	{
		for(uint32 x = rect.x; x < endX; x++)
		{
			uint32 pageIndex = x + (y * areaRect.width);
			assert(pageIndex < sizeof(m_dirtyPages) * 8);
			assert(pageIndex < GetPageCount());
			unsigned int dirtyPageSection = pageIndex / (sizeof(m_dirtyPages[0]) * 8);
			unsigned int dirtyPageIndex = pageIndex % (sizeof(m_dirtyPages[0]) * 8);
			m_dirtyPages[dirtyPageSection] &= ~(1ULL << dirtyPageIndex);
		}
	}
}
