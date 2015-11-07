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
	POINT    m_pressPosition;
	POINT    m_offset;
};
