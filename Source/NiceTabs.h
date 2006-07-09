#ifndef _NICETABS_H_
#define _NICETABS_H_

#include "win32/CustomDrawn.h"
#include "List.h"
#include "Event.h"

struct TABITEM
{
	xchar*			sCaption;
	unsigned long	nWidth;
	unsigned long	nFlags;
};

enum TABFLAGS
{
	TAB_FLAG_UNDELETEABLE	= 0x01,
	TAB_FLAG_UNMOVABLE		= 0x02,
};

class CNiceTabs : public Framework::CCustomDrawn
{
public:
										CNiceTabs(HWND, RECT*);
										~CNiceTabs();
	void								InsertTab(xchar*, unsigned long, unsigned int);

	Framework::CEvent<unsigned int>		m_OnTabChange;

protected:
	void								Paint(HDC);
	long								OnMouseLeave();
	long								OnMouseMove(WPARAM, int, int);
	long								OnLeftButtonDown(int, int);
	long								OnLeftButtonUp(int, int);

private:
	HFONT								CreateOurFont();
	unsigned long						GetTabWidth(unsigned int);
	unsigned long						GetTabBase(unsigned int);
	unsigned long						MeasureString(xchar*);

	Framework::CList<TABITEM>			m_List;
	Framework::CList<TABITEM>::INDEXOR	m_ListIdx;
	HBITMAP								m_nEx;
	HBITMAP								m_nExd;

	unsigned long						m_nSelected;
	unsigned int						m_nHoverEx;
	unsigned int						m_nLButtonEx;

};

#endif