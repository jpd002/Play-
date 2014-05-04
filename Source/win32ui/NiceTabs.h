#pragma once

#include <vector>
#include <boost/signals2.hpp>
#include "win32/CustomDrawn.h"

class CNiceTabs : public Framework::Win32::CCustomDrawn
{
public:
	enum TABFLAGS
	{
		TAB_FLAG_UNDELETEABLE	= 0x01,
		TAB_FLAG_UNMOVABLE		= 0x02,
	};

	typedef boost::signals2::signal<void (unsigned int)> TabChangeSignal;

										CNiceTabs(HWND, const RECT&);
	virtual								~CNiceTabs();
	void								InsertTab(const TCHAR*, unsigned long, unsigned int);

	TabChangeSignal						OnTabChange;

protected:
	void								Paint(HDC);
	long								OnMouseLeave() override;
	long								OnMouseMove(WPARAM, int, int) override;
	long								OnLeftButtonDown(int, int) override;
	long								OnLeftButtonUp(int, int) override;

private:
	struct TABITEM
	{
		std::tstring	sCaption;
		unsigned long	nWidth;
		unsigned long	nFlags;
		unsigned long	nID;
	};

	typedef std::vector<TABITEM> TabItemList;

	HFONT								CreateOurFont();
	unsigned long						GetTabWidth(unsigned int);
	unsigned long						GetTabBase(unsigned int);
	unsigned long						MeasureString(const TCHAR*);

	TabItemList							m_List;
	HBITMAP								m_nEx;
	HBITMAP								m_nExd;

	unsigned int						m_nSelected;
	unsigned int						m_nHoverEx;
	unsigned int						m_nLButtonEx;

};
