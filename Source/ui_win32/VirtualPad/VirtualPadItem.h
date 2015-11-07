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
	virtual void    OnMouseDown(int, int);
	virtual void    OnMouseMove(int, int);
	virtual void    OnMouseUp();

	Framework::Win32::CRect    GetBounds() const;
	void                       SetBounds(const Framework::Win32::CRect&);

	uint32    GetPointerId() const;
	void      SetPointerId(uint32);

protected:
	Framework::Win32::CRect    m_bounds = Framework::Win32::CRect(0, 0, 0, 0);

	uint32    m_pointerId = 0;
};
