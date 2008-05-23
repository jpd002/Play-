#include "RegViewPage.h"

#define CLSNAME			_T("CRegViewPage")
#define XMARGIN			6
#define YMARGIN			5
#define YSPACE			4

using namespace Framework;

CRegViewPage::CRegViewPage(HWND hParent, RECT* pR)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground	= NULL;
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&w);
	}

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_CHILD | WS_VSCROLL, pR, hParent, NULL);
	SetClassPtr();
}

CRegViewPage::~CRegViewPage()
{
	
}

void CRegViewPage::SetDisplayText(const char* sText)
{
	m_sText = sText;
	UpdateScroll();
}

void CRegViewPage::Update()
{
	UpdateScroll();
	Redraw();
}

long CRegViewPage::OnVScroll(unsigned int nType, unsigned int nThumbPos)
{
	SCROLLINFO si;
	unsigned int nPosition;
	
	nPosition = GetScrollPosition();
	switch(nType)
	{
	case SB_LINEDOWN:
		nPosition++;
		break;
	case SB_LINEUP:
		nPosition--;
		break;
	case SB_PAGEDOWN:
		nPosition += 10;
		break;
	case SB_PAGEUP:
		nPosition -= 10;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nPosition = GetScrollThumbPosition();
		break;
	default:
		return FALSE;
		break;
	}

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= nPosition;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	Redraw();
	return TRUE;	
}

long CRegViewPage::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
    Win32::CCustomDrawn::OnSize(nX, nY, nType);
	Update();
	return TRUE;
}

long CRegViewPage::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		OnVScroll(SB_LINEDOWN, 0);
	}
	else
	{
		OnVScroll(SB_LINEUP, 0);
	}
	return TRUE;
}

long CRegViewPage::OnLeftButtonDown(int nX, int nY)
{
	SetFocus();
	return TRUE;
}

unsigned int CRegViewPage::GetLineCount(const char* sText)
{
	const char* sNext;
	unsigned int nLines;

	nLines = 0;

	sNext = strchr(sText, '\n');
	while(sNext != NULL)
	{
		nLines++;
		sNext = strchr(sNext + 1, '\n');
	}

	return nLines;
}

unsigned int CRegViewPage::GetFontHeight()
{
	HDC hDC;
	HFONT nFont;
	SIZE s;

	hDC = GetDC(m_hWnd);
	nFont = GetFont();
	SelectObject(hDC, nFont);

	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	DeleteObject(nFont);

	ReleaseDC(m_hWnd, hDC);

	return s.cy;
}

unsigned int CRegViewPage::GetVisibleLineCount()
{
	unsigned int nFontCY, nLines;
	RECT rwin;

	GetClientRect(&rwin);

	nFontCY = GetFontHeight();

	nLines = (rwin.bottom - (YMARGIN * 2)) / (nFontCY + YSPACE);

	return nLines;
}

void CRegViewPage::Paint(HDC hDC)
{
	unsigned int nScrollPos, nCurrent, nTotal, nCount, nX, nY, nFontCY;
	const char* sLine;
	const char* sNext;
	RECT rwin;
	HFONT nFont;

	GetClientRect(&rwin);

	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	nFont = GetFont();
	nFontCY = GetFontHeight();
	nTotal = GetVisibleLineCount();
	nScrollPos = GetScrollPosition();

	SelectObject(hDC, nFont);

	nCurrent = 0;
	nCount = 0;
	nX = XMARGIN;
	nY = YMARGIN;

	sLine = m_sText.c_str();
	while(sLine != NULL)
	{
		sNext = strchr(sLine, '\n');
		if(sNext != NULL)
		{
			sNext++;
		}

		if(nCurrent < nScrollPos)
		{
			nCurrent++;
			sLine = sNext;
			continue;
		}

		TextOutA(hDC, nX, nY, sLine, (int)(sNext - sLine - 2));
		nY += (nFontCY + YSPACE);

		nCurrent++;
		sLine = sNext;

		nCount++;
		if(nCount >= nTotal)
		{
			break;
		}
	}

	DeleteObject(nFont);
}

void CRegViewPage::UpdateScroll()
{
	unsigned int nTotal;
	SCROLLINFO si;

	nTotal = GetLineCount(m_sText.c_str()) - GetVisibleLineCount();

	if((int)nTotal < 0)
	{
		nTotal = 0;
	}

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_RANGE;
	si.nMin		= 0;
	si.nMax		= nTotal;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
}

HFONT CRegViewPage::GetFont()
{
	return CreateFont(-11, 0, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, _T("Courier New"));
}

unsigned int CRegViewPage::GetScrollPosition()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nPos;
}

unsigned int CRegViewPage::GetScrollThumbPosition()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_TRACKPOS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nTrackPos;
}
