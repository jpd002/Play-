#pragma once

#include "win32/CustomDrawn.h"
#include "win32/GdiObj.h"
#include <string>

class CRegViewPage : public Framework::Win32::CCustomDrawn
{
public:
									CRegViewPage(HWND, RECT*);
	virtual							~CRegViewPage();

protected:
	void							SetDisplayText(const char*);
	void							Update();
	long							OnVScroll(unsigned int, unsigned int);
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnMouseWheel(short);
	long							OnLeftButtonDown(int, int);

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
