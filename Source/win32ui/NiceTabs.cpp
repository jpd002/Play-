#include "NiceTabs.h"
#include "resource.h"
#include <assert.h>

#define CLASSNAME	_T("CNiceTabs")
#define EXLEFT(r)	((r.right - 23) + 7)
#define EXTOP		8
#define EXWIDTH		8
#define EXHEIGHT	7
#define EXRIGHT(r)	(EXLEFT(r) + EXWIDTH)
#define EXBOTTOM	(EXTOP + EXHEIGHT)

CNiceTabs::CNiceTabs(HWND hParent, RECT* pR) :
m_nSelected(0)
{
	if(!DoesWindowClassExist(CLASSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLASSNAME;
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&w);
	}

	Create(NULL, CLASSNAME, CLASSNAME, WS_VISIBLE | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();

	m_nEx	= LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_EX));
	m_nExd	= LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_EXD));

	m_nHoverEx = 0;
	m_nLButtonEx = 0;
}

CNiceTabs::~CNiceTabs()
{
	DeleteObject(m_nEx);
    DeleteObject(m_nExd);
}

void CNiceTabs::InsertTab(const TCHAR* sCaption, unsigned long nFlags, unsigned int nID)
{
    TABITEM t;
    t.sCaption  = sCaption;
	t.nWidth	= MeasureString(sCaption);
	t.nFlags	= nFlags; 
    t.nID       = nID;
    m_List.push_back(t);
}

unsigned long CNiceTabs::GetTabWidth(unsigned int index)
{
	const TABITEM& t(m_List[index]);
	return t.nWidth + 24 + 5;
}

unsigned long CNiceTabs::GetTabBase(unsigned int index)
{
	unsigned long nBase = 4;
    for(unsigned int i = 0; i < index; i++)
    {
		nBase += GetTabWidth(i);
    }
	return nBase;
}

unsigned long CNiceTabs::MeasureString(const TCHAR* sText)
{
	HDC hDC;
	SIZE p;
	HFONT nFont;

	hDC = GetDC(m_hWnd);
	
	nFont = CreateOurFont();
	SelectObject(hDC, nFont);

	GetTextExtentPoint32(hDC, sText, (int)_tcslen(sText), &p);

	DeleteObject(nFont);
	ReleaseDC(m_hWnd, hDC);

	return p.cx;
}

HFONT CNiceTabs::CreateOurFont()
{
	return CreateFont(-11, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, _T("Tahoma"));
}

void CNiceTabs::Paint(HDC hDC)
{
	HDC hMem;
	HBRUSH nBrush;
	HPEN nPen;
	HFONT nFont;
	RECT rcli, rc;

	GetClientRect(&rcli);

	nFont = CreateOurFont();
	SelectObject(hDC, nFont);
	SetBkMode(hDC, TRANSPARENT);

	//Draw the background
	nBrush = CreateSolidBrush(RGB(0xF7, 0xF3, 0xE9));
	FillRect(hDC, &rcli, nBrush);
	DeleteObject(nBrush);

	nPen = CreatePen(PS_SOLID, 0, RGB(0x00, 0x00, 0x00));
	SelectObject(hDC, nPen);
	MoveToEx(hDC, rcli.left, 0, NULL);
	LineTo(hDC, rcli.right, 0);
	DeleteObject(nPen);

	//Draw the individual tabs
    for(unsigned int i = 0; i < m_List.size(); i++)
	{
	    TABITEM& t(m_List[i]);
		unsigned long nWidth = GetTabWidth(i);
		unsigned long nBase = GetTabBase(i);

		if(i == m_nSelected)
		{
			SetRect(&rc, nBase, 0, nBase + nWidth, 19);
			nBrush = CreateSolidBrush(RGB(0xD4, 0xD0, 0xC8));
			FillRect(hDC, &rc, nBrush);

			nPen = CreatePen(PS_SOLID, 0, RGB(0x00, 0x00, 0x00));
			SelectObject(hDC, nPen);

			MoveToEx(hDC, nBase, 19, NULL);
			LineTo(hDC, nWidth + nBase, 19);

			MoveToEx(hDC, nWidth + nBase, 0, NULL);
			LineTo(hDC, nWidth + nBase, 20);

			DeleteObject(nPen);

			nPen = CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0xFF));
			SelectObject(hDC, nPen);
			MoveToEx(hDC, nBase, 0, NULL);
			LineTo(hDC, nBase, 19);
			DeleteObject(nPen);
			
			SetTextColor(hDC, RGB(0x00, 0x00, 0x00));
			TextOut(hDC, nBase + 24, 4, t.sCaption.c_str(), t.sCaption.length());
		}
		else
		{

			SetTextColor(hDC, RGB(0x80, 0x80, 0x80));
			TextOut(hDC, nBase + 24, 4, t.sCaption.c_str(), t.sCaption.length());
			if(i != (m_nSelected - 1))
			{
				//Draw the right line
				nPen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
				SelectObject(hDC, nPen);
				MoveToEx(hDC, nBase + nWidth - 1, 3, NULL);
				LineTo(hDC, nBase + nWidth - 1, 17);
				DeleteObject(nPen);
			}

		}
	}

    if(m_List.size() != 0)
    {
	    TABITEM& t(m_List[m_nSelected]);
	    if(!(t.nFlags & TAB_FLAG_UNDELETEABLE))
	    {
		    hMem = CreateCompatibleDC(hDC);
		    SelectObject(hMem, m_nEx);
		    BitBlt(hDC, EXLEFT(rcli), EXTOP, EXWIDTH, EXHEIGHT, hMem, 0, 0, SRCCOPY);
		    DeleteDC(hMem);

		    if(m_nHoverEx)
		    {
			    if(m_nLButtonEx)
			    {
				    nPen = CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0xFF));
			    }
			    else
			    {
				    nPen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
			    }

			    SelectObject(hDC, nPen);

			    MoveToEx(hDC, EXRIGHT(rcli) + 3, EXBOTTOM + 2, NULL);
			    LineTo(hDC, EXRIGHT(rcli) + 3, EXTOP - 3);

			    MoveToEx(hDC, EXRIGHT(rcli) + 3, EXBOTTOM + 2, NULL);
			    LineTo(hDC, EXLEFT(rcli) - 3, EXBOTTOM + 2);
			
			    DeleteObject(nPen);

			    if(m_nLButtonEx)
			    {
				    nPen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
			    }
			    else
			    {
				    nPen = CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0xFF));
			    }

			    SelectObject(hDC, nPen);

			    MoveToEx(hDC, EXLEFT(rcli) - 3, EXTOP - 3, NULL);
			    LineTo(hDC, EXLEFT(rcli) - 3, EXBOTTOM + 2);

			    MoveToEx(hDC, EXLEFT(rcli) - 3, EXTOP - 3, NULL);
			    LineTo(hDC, EXRIGHT(rcli) + 3, EXTOP - 3);

			    DeleteObject(nPen);
		    }
	    }
    }

	DeleteObject(nFont);

}

long CNiceTabs::OnMouseLeave()
{
	if(m_nHoverEx != 0)
	{
		m_nHoverEx = 0;
		RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
	}
	if(m_nLButtonEx != 0)
	{
		m_nLButtonEx = 0;
		RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
	}
	return FALSE;
}

long CNiceTabs::OnMouseMove(WPARAM nButton, int nX, int nY)
{
	RECT rcli;

	GetClientRect(&rcli);

	if((nX >= EXLEFT(rcli)) && (nX <= EXRIGHT(rcli)))
	{
		if((nY >= EXTOP) && (nY <= EXBOTTOM))
		{
			if(m_nHoverEx != 1)
			{
				m_nHoverEx = 1;
				RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
			}
		}
		else
		{
			if(m_nHoverEx != 0)
			{
				m_nHoverEx = 0;
				RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
			}
		}
	}
	else
	{
		if(m_nHoverEx != 0)
		{
			m_nHoverEx = 0;
			RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}
	}

	if(m_nHoverEx == 1)
	{
		if((m_nLButtonEx != 1) && (nButton & MK_LBUTTON))
		{
			m_nLButtonEx = 1;
			RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}
		if((m_nLButtonEx != 0) && !(nButton & MK_LBUTTON))
		{
			m_nLButtonEx = 0;
			RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}
	}
	return FALSE;
}

long CNiceTabs::OnLeftButtonDown(int nX, int nY)
{
	int nBase = GetTabBase(0);
	for(unsigned int i = 0; i < m_List.size(); i++)
	{
		int nWidth = GetTabWidth(i);
		if((nX > nBase) && (nX < (nBase + nWidth)))
		{
			m_nSelected = i;
			m_OnTabChange(m_List[i].nID);
			RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
			break;
		}
		nBase += nWidth;
	}

	if(m_nHoverEx == 1)
	{
		if(m_nLButtonEx != 1)
		{
			m_nLButtonEx = 1;
			RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
		}
	}
	return FALSE;
}

long CNiceTabs::OnLeftButtonUp(int nX, int nY)
{
	if(m_nHoverEx == 1 && m_nSelected != 0)
	{
		m_nLButtonEx = 0;

		TABITEM& t(m_List[m_nSelected]);
		if(t.nFlags & TAB_FLAG_UNDELETEABLE)
		{
			return FALSE;
		}
        m_List.erase(m_List.begin() + m_nSelected);

		//Always safe to go back, since the first tab will never be deleted
		m_nSelected--;

		m_OnTabChange(m_List[m_nSelected].nID);
		RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
	}
	return FALSE;
}
