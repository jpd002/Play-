#pragma once

#include <utility>
#include "Types.h"

class CGsCachedArea
{
public:
	typedef uint64 DirtyPageHolder;

	enum
	{
		MAX_DIRTYPAGES_SECTIONS = 8,
		MAX_DIRTYPAGES = sizeof(DirtyPageHolder) * 8 * MAX_DIRTYPAGES_SECTIONS
	};

								CGsCachedArea();

	void						SetArea(uint32 psm, uint32 bufPtr, uint32 bufWidth, uint32 height);

	std::pair<uint32, uint32>	GetPageRect() const;
	uint32						GetPageCount() const;
	uint32						GetSize() const;

	void						Invalidate(uint32, uint32);
	bool						IsPageDirty(uint32) const;
	void						SetPageDirty(uint32);
	bool						HasDirtyPages() const;
	void						ClearDirtyPages();

private:
	uint32						m_psm = 0;
	uint32						m_bufPtr = 0;
	uint32						m_bufWidth = 0;
	uint32						m_height = 0;

	DirtyPageHolder				m_dirtyPages[MAX_DIRTYPAGES_SECTIONS];
};
