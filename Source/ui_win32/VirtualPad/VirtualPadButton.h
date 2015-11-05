#pragma once

#include <string>
#include "VirtualPadItem.h"

class CVirtualPadButton : public CVirtualPadItem
{
public:
	virtual         ~CVirtualPadButton();
	
	virtual void    Draw(Gdiplus::Graphics&) override;
	virtual void    OnMouseDown(int, int) override;
	virtual void    OnMouseUp() override;

	void    SetCaption(const std::wstring&);

protected:
	std::wstring    m_caption;
	bool            m_pressed = false;
};
