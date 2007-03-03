#ifndef _REGVIEWPAGE_H_
#define _REGVIEWPAGE_H_

#include "win32/CustomDrawn.h"
#include "Str.h"

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

	unsigned int					GetLineCount(const char*);
	unsigned int					GetVisibleLineCount();
	unsigned int					GetFontHeight();
	void							UpdateScroll();

	HFONT							GetFont();
	
	unsigned int					GetScrollPosition();
	unsigned int					GetScrollThumbPosition();

	Framework::CStrA				m_sText;
};

#endif
