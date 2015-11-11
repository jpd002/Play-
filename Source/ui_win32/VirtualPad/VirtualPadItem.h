#pragma once

#include <Windows.h>
#include <minmax.h>
#include <GdiPlus.h>

class CVirtualPadItem
{
public:
	virtual         ~CVirtualPadItem();

	virtual void    Draw(Gdiplus::Graphics&) = 0;
	virtual void    OnMouseDown(int, int);
	virtual void    OnMouseMove(int, int);
	virtual void    OnMouseUp();

	Gdiplus::RectF    GetBounds() const;
	void              SetBounds(const Gdiplus::RectF&);

	void    SetImage(Gdiplus::Bitmap*);

	uint32    GetPointerId() const;
	void      SetPointerId(uint32);

protected:
	Gdiplus::RectF      m_bounds = Gdiplus::RectF(0, 0, 0, 0);
	Gdiplus::Bitmap*    m_image = nullptr;
	uint32              m_pointerId = 0;
};
