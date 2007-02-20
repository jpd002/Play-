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
m_ListIdx(&m_List)
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

	m_nSelected = 0;

	m_nEx	= LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_EX));
	m_nExd	= LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_EXD));

	m_nHoverEx = 0;
	m_nLButtonEx = 0;
}

CNiceTabs::~CNiceTabs()
{
	TABITEM* t;
	while(m_List.Count())
	{
		t = m_List.Pull();
		free(t->sCaption);
		free(t);
	}
	DeleteObject(m_nEx);
}

void CNiceTabs::InsertTab(const TCHAR* sCaption, unsigned long nFlags, unsigned int nID)
{
	TABITEM* t;
	t = (TABITEM*)malloc(sizeof(TABITEM));
	t->sCaption = (TCHAR*)malloc((_tcslen(sCaption) + 1) * sizeof(TCHAR));
	_tcscpy(t->sCaption, sCaption);
	t->nWidth	= MeasureString(t->sCaption);
	t->nFlags	= nFlags; 
	m_List.Insert(t, nID);
	m_ListIdx.Reset();
}

unsigned long CNiceTabs::GetTabWidth(unsigned int nTab)
{
	TABITEM* t;
	t = m_ListIdx.GetAt(nTab);
	assert(t);
	return t->nWidth + 24 + 5;
}

unsigned long CNiceTabs::GetTabBase(unsigned int nTab)
{
	unsigned long nBase, i;
	nBase = 4;
	for(i = 0; i < nTab; i++)
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
	unsigned long nBase, nWidth;
	TABITEM* t;
	unsigned int i;

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
	for(i = 0; i < m_List.Count(); i++)
	{
		t = m_ListIdx.GetAt(i);
		nWidth = GetTabWidth(i);
		nBase = GetTabBase(i);
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
			TextOut(hDC, nBase + 24, 4, t->sCaption, (int)_tcslen(t->sCaption));
		}
		else
		{

			SetTextColor(hDC, RGB(0x80, 0x80, 0x80));
			TextOut(hDC, nBase + 24, 4, t->sCaption, (int)_tcslen(t->sCaption));
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

	t = m_ListIdx.GetAt(m_nSelected);
	if(t != NULL)
	{

		if(!(t->nFlags & TAB_FLAG_UNDELETEABLE))
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
	int nBase, nWidth;
	
	nBase = GetTabBase(0);
	for(unsigned int i = 0; i < m_List.Count(); i++)
	{
		nWidth = GetTabWidth(i);
		if((nX > nBase) && (nX < (nBase + nWidth)))
		{
			m_nSelected = i;
			m_OnTabChange(m_ListIdx.KeyAt(i));
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
	if(m_nHoverEx == 1)
	{
		TABITEM* i;
		m_nLButtonEx = 0;

		i = m_ListIdx.GetAt(m_nSelected);
		if(i->nFlags & TAB_FLAG_UNDELETEABLE)
		{
			return FALSE;
		}
		m_List.Remove(m_ListIdx.KeyAt(m_nSelected));
		m_ListIdx.Reset();

		//Always safe to go back, since the first tab will never be deleted
		m_nSelected--;

		free(i->sCaption);
		free(i);

		m_OnTabChange(m_ListIdx.KeyAt(m_nSelected));
		RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
	}
	return FALSE;
}
