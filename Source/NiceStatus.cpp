#include "NiceStatus.h"
#include "PtrMacro.h"

#define CLASSNAME _X("CNiceStatus")
#define DISTBETW  (2)

using namespace Framework;

CNiceStatus::CNiceStatus(HWND hParent, RECT* pR)
{
	if(!DoesWindowClassExist(CLASSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= WndProc;
		w.lpszClassName	= CLASSNAME;
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&w);
	}

	Create(NULL, CLASSNAME, CLASSNAME, WS_VISIBLE | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();
}

CNiceStatus::~CNiceStatus()
{
	STATUSPANEL* p;

	while(m_Panel.Count() != 0)
	{
		p = m_Panel.Pull();
		free(p->sCaption);
		free(p);
	}
}

HFONT CNiceStatus::CreateOurFont()
{
	return CreateFont(-11, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, _X("Tahoma"));
}

void CNiceStatus::InsertPanel(unsigned long nID, double nWidth, xchar* sCaption)
{
	STATUSPANEL* p;
	p = (STATUSPANEL*)malloc(sizeof(STATUSPANEL));
	p->nType		= PANELTYPE_TEXT;
	p->nWidth		= nWidth;
	p->sCaption		= (xchar*)malloc((xstrlen(sCaption) + 1) * sizeof(xchar));
	xstrcpy(p->sCaption, sCaption);
	m_Panel.Insert(p, nID);
}

void CNiceStatus::SetCaption(unsigned long nID, xchar* sCaption)
{
	STATUSPANEL* p;

	p = m_Panel.Find(nID);
	if(p == NULL) return;

	FREEPTR(p->sCaption);
	p->sCaption = (xchar*)malloc((xstrlen(sCaption) + 1) * sizeof(xchar));
	xstrcpy(p->sCaption, sCaption);

	InvalidateRect(m_hWnd, NULL, FALSE);
}

void CNiceStatus::Paint(HDC hDC)
{
	RECT rwin;
	HFONT nFont;
	HPEN nPen;
	STATUSPANEL* p;
	unsigned long nWidth;
	unsigned long nX;
	CList<STATUSPANEL>::ITERATOR itPanel;

	GetClientRect(&rwin);

	//Draw the background
	nFont = CreateOurFont();

	FillRect(hDC, &rwin, GetSysColorBrush(COLOR_BTNFACE));

	nPen = CreatePen(PS_SOLID, 0, RGB(0x80, 0x80, 0x80));
//	nBrush = CreateSolidBrush(RGB(0xD4, 0xD0, 0xC8));

	SelectObject(hDC, GetSysColorBrush(COLOR_BTNFACE));
	SelectObject(hDC, nPen);
	SelectObject(hDC, nFont);
	SetBkMode(hDC, TRANSPARENT);
	nX = 0;

	for(itPanel = m_Panel.Begin(); itPanel.HasNext(); itPanel++)
	{
		p = (*itPanel);
		nWidth = (unsigned long)(p->nWidth * (rwin.right - ((m_Panel.Count() - 1) * DISTBETW)));
		Rectangle(hDC, nX, 0, nX + nWidth, rwin.bottom);
		TextOut(hDC, nX + 6, 3, p->sCaption, (int)xstrlen(p->sCaption));
		nX += nWidth + DISTBETW;
	}

	DeleteObject(nPen);
	DeleteObject(nFont);
}
