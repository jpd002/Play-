#include <cassert>
#include "GsCachedArea.h"
#include "GsPixelFormats.h"

static bool DoMemoryRangesOverlap(uint32 start1, uint32 size1, uint32 start2, uint32 size2)
{
	uint32 min1 = start1;
	uint32 max1 = start1 + size1;

	uint32 min2 = start2;
	uint32 max2 = start2 + size2;

	if(max1 < min2) return false;
	if(min1 > max2) return false;
	
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

std::pair<uint32, uint32> CGsCachedArea::GetPageRect() const
{
	auto texturePageSize = CGsPixelFormats::GetPsmPageSize(m_psm);
	uint32 pageCountX = (m_bufWidth + texturePageSize.first - 1) / texturePageSize.first;
	uint32 pageCountY = (m_height + texturePageSize.second - 1) / texturePageSize.second;
	return std::make_pair(pageCountX, pageCountY);
}

uint32 CGsCachedArea::GetPageCount() const
{
	auto pageRect = GetPageRect();
	return pageRect.first * pageRect.second;
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
		uint32 pageCount = std::min<uint32>(GetPageCount(), (memorySize + CGsPixelFormats::PAGESIZE - 1) / CGsPixelFormats::PAGESIZE);

		for(unsigned int i = 0; i < pageCount; i++)
		{
			SetPageDirty(pageStart + i);
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
