#include "RegViewPage.h"

#define CLSNAME			_T("CRegViewPage")
#define XMARGIN			6
#define YMARGIN			5
#define YSPACE			4

CRegViewPage::CRegViewPage(HWND hParent, const RECT& rect)
: m_font(CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, _T("Courier New")))
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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_CHILD | WS_VSCROLL, rect, hParent, NULL);
	SetClassPtr();
}

CRegViewPage::~CRegViewPage()
{
	
}

void CRegViewPage::SetDisplayText(const char* text)
{
	m_text = text;
	UpdateScroll();
}

void CRegViewPage::Update()
{
	UpdateScroll();
	Redraw();
}

long CRegViewPage::OnVScroll(unsigned int nType, unsigned int nThumbPos)
{
	unsigned int nPosition = GetScrollPosition();
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

	SCROLLINFO si;
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
	Framework::Win32::CCustomDrawn::OnSize(nX, nY, nType);
	Update();
	return TRUE;
}

long CRegViewPage::OnMouseWheel(int x, int y, short z)
{
	if(z < 0)
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
	unsigned int nLines = 0;

	const char* sNext = strchr(sText, '\n');
	while(sNext != NULL)
	{
		nLines++;
		sNext = strchr(sNext + 1, '\n');
	}

	return nLines;
}

unsigned int CRegViewPage::GetFontHeight()
{
	HDC hDC = GetDC(m_hWnd);
	SelectObject(hDC, m_font);

	SIZE s;
	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	ReleaseDC(m_hWnd, hDC);

	return s.cy;
}

unsigned int CRegViewPage::GetVisibleLineCount()
{
	RECT rwin = GetClientRect();
	unsigned int nFontCY = GetFontHeight();
	unsigned int nLines = (rwin.bottom - (YMARGIN * 2)) / (nFontCY + YSPACE);
	return nLines;
}

void CRegViewPage::Paint(HDC hDC)
{
	RECT rwin = GetClientRect();

	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	unsigned int nFontCY = GetFontHeight();
	unsigned int nTotal = GetVisibleLineCount();
	unsigned int nScrollPos = GetScrollPosition();

	SelectObject(hDC, m_font);

	unsigned int nCurrent = 0;
	unsigned int nCount = 0;
	unsigned int nX = XMARGIN;
	unsigned int nY = YMARGIN;

	const char* sLine = m_text.c_str();
	while(sLine != NULL)
	{
		const char* sNext = strchr(sLine, '\n');
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

		int textLength = (sNext == NULL) ? strlen(sLine) : static_cast<int>(sNext - sLine - 2);
		DrawTextA(hDC, sLine, textLength, Framework::Win32::CRect(nX, nY, nX, nY), DT_NOCLIP | DT_EXPANDTABS);
		nY += (nFontCY + YSPACE);

		nCurrent++;
		sLine = sNext;

		nCount++;
		if(nCount >= nTotal)
		{
			break;
		}
	}
}

void CRegViewPage::UpdateScroll()
{
	int nTotal = GetLineCount(m_text.c_str()) - GetVisibleLineCount();

	if(nTotal < 0)
	{
		nTotal = 0;
	}

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_RANGE;
	si.nMin		= 0;
	si.nMax		= nTotal;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
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
