#pragma once

#include "win32/CustomDrawn.h"
#include "win32/GdiObj.h"
#include <string>

class CRegViewPage : public Framework::Win32::CCustomDrawn
{
public:
									CRegViewPage(HWND, const RECT&);
	virtual							~CRegViewPage();

	virtual void					Update();

protected:
	void							SetDisplayText(const char*);
	long							OnVScroll(unsigned int, unsigned int) override;
	long							OnSize(unsigned int, unsigned int, unsigned int) override;
	long							OnMouseWheel(int, int, short) override;
	long							OnLeftButtonDown(int, int) override;

private:
	struct RENDERMETRICS
	{
		int xmargin = 0;
		int yspace = 0;
		int ymargin = 0;
		int fontSizeX = 0;
		int fontSizeY = 0;
	};

	void							Paint(HDC);

	static unsigned int				GetLineCount(const char*);
	unsigned int					GetVisibleLineCount();
	void							UpdateScroll();

	unsigned int					GetScrollPosition();
	unsigned int					GetScrollThumbPosition();

	Framework::Win32::CFont			m_font;
	std::string						m_text;
	RENDERMETRICS					m_renderMetrics;
};
