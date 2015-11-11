#pragma once

#include "VirtualPadItem.h"

class CVirtualPadStick : public CVirtualPadItem
{
public:
	virtual         ~CVirtualPadStick();

	virtual void    Draw(Gdiplus::Graphics&) override;
	virtual void    OnMouseDown(int, int) override;
	virtual void    OnMouseMove(int, int) override;
	virtual void    OnMouseUp() override;

private:
	Gdiplus::PointF    m_pressPosition = Gdiplus::PointF(0, 0);
	Gdiplus::PointF    m_offset = Gdiplus::PointF(0, 0);
};
