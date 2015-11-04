#pragma once

#include <Windows.h>
#include <minmax.h>
#include <GdiPlus.h>
#include "win32/Rect.h"

class CVirtualPadItem
{
public:
	virtual         ~CVirtualPadItem();

	virtual void    Draw(Gdiplus::Graphics&) = 0;
	virtual void    OnMouseDown(int, int) = 0;

	void            SetBounds(const Framework::Win32::CRect&);

protected:
	Framework::Win32::CRect    m_bounds = Framework::Win32::CRect(0, 0, 0, 0);
};
