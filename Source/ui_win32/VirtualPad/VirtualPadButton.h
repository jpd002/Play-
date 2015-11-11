#pragma once

#include <string>
#include "VirtualPadItem.h"

class CVirtualPadButton : public CVirtualPadItem
{
public:
	                CVirtualPadButton();
	                CVirtualPadButton(const CVirtualPadButton&) = delete;
	virtual         ~CVirtualPadButton();
	
	CVirtualPadButton&    operator =(const CVirtualPadButton&) = delete;

	virtual void    Draw(Gdiplus::Graphics&) override;
	virtual void    OnMouseDown(int, int) override;
	virtual void    OnMouseUp() override;

	void    SetCaption(const std::wstring&);

protected:
	std::wstring     m_caption;
	bool             m_pressed = false;
	Gdiplus::Font    m_font;

};
