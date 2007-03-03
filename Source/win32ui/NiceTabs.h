#ifndef _NICETABS_H_
#define _NICETABS_H_

#include <boost/signal.hpp>
#include "win32/CustomDrawn.h"
#include "List.h"

struct TABITEM
{
	TCHAR*			sCaption;
	unsigned long	nWidth;
	unsigned long	nFlags;
};

enum TABFLAGS
{
	TAB_FLAG_UNDELETEABLE	= 0x01,
	TAB_FLAG_UNMOVABLE		= 0x02,
};

class CNiceTabs : public Framework::Win32::CCustomDrawn
{
public:
										CNiceTabs(HWND, RECT*);
	virtual								~CNiceTabs();
	void								InsertTab(const TCHAR*, unsigned long, unsigned int);

	boost::signal<void (unsigned int)>	m_OnTabChange;

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
	unsigned long						MeasureString(const TCHAR*);

	Framework::CList<TABITEM>			m_List;
	Framework::CList<TABITEM>::INDEXOR	m_ListIdx;
	HBITMAP								m_nEx;
	HBITMAP								m_nExd;

	unsigned long						m_nSelected;
	unsigned int						m_nHoverEx;
	unsigned int						m_nLButtonEx;

};

#endif