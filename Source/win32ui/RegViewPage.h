#pragma once

#include "win32/CustomDrawn.h"
#include "win32/GdiObj.h"
#include <string>

class CRegViewPage : public Framework::Win32::CCustomDrawn
{
public:
									CRegViewPage(HWND, const RECT&);
	virtual							~CRegViewPage();

protected:
	void							SetDisplayText(const char*);
	void							Update();
	long							OnVScroll(unsigned int, unsigned int) override;
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnMouseWheel(int, int, short) override;
	long							OnLeftButtonDown(int, int) override;

private:
	void							Paint(HDC);

	static unsigned int				GetLineCount(const char*);
	unsigned int					GetVisibleLineCount();
	unsigned int					GetFontHeight();
	void							UpdateScroll();

	unsigned int					GetScrollPosition();
	unsigned int					GetScrollThumbPosition();

	Framework::Win32::CFont			m_font;
	std::string						m_text;
};
