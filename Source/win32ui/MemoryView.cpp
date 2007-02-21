#include <stdio.h>
#include <string.h>
#include "MemoryView.h"
#include "string_cast.h"

#define CLSNAME		_T("CMemoryView")
#define XMARGIN		5
#define YMARGIN		5
#define YSPACE		0

using namespace std;

CMemoryView::CMemoryView(HWND hParent, RECT* pR)
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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, pR, hParent, NULL);
	SetClassPtr();

	m_nSize = 0;

	UpdateScrollRange();
}

CMemoryView::~CMemoryView()
{

}

HFONT CMemoryView::GetFont()
{
	return (HFONT)GetStockObject(ANSI_FIXED_FONT);
}

void CMemoryView::ScrollToAddress(uint32 nAddress)
{
	unsigned int nRow;
	unsigned int nRows, nCols;
	SCROLLINFO si;

	GetVisibleRowsCols(&nRows, &nCols);

	//Humm, nvm...
	if(nCols == 0) return;
	if(nAddress >= m_nSize) return;

	nRow = (nAddress / nCols);

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= nRow;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	Redraw();
}

void CMemoryView::UpdateScrollRange()
{
	unsigned int nTotalRows;
	unsigned int nCols, nRows;
	SCROLLINFO si;

	GetVisibleRowsCols(&nRows, &nCols);

	if(nCols != 0)
	{
		nTotalRows = (m_nSize / nCols);
		if((m_nSize % nCols) != 0) nTotalRows++;
	}
	else
	{
		nTotalRows = 0;
	}

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nMin			= 0;
	if(nTotalRows <= nRows)
	{
		si.nMax		= 0;
	}
	else
	{
		si.nMax		= nTotalRows - nRows;
	}
	si.fMask		= SIF_RANGE | SIF_DISABLENOSCROLL;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE); 
}

unsigned int CMemoryView::GetScrollOffset()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nPos;
}

unsigned int CMemoryView::GetScrollThumbPosition()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize	= sizeof(SCROLLINFO);
	si.fMask	= SIF_TRACKPOS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nTrackPos;
}

void CMemoryView::GetVisibleRowsCols(unsigned int* pRows, unsigned int* pCols)
{
	HDC hDC;
	SIZE s;
	RECT rc;
	unsigned int nRows, nCols;
	HFONT hFont;

	hDC = GetDC(m_hWnd);
	hFont = GetFont();

	SelectObject(hDC, hFont);

	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	DeleteObject(hFont);
	ReleaseDC(m_hWnd, hDC);

	GetClientRect(&rc);

	nRows = (rc.bottom - (YMARGIN * 2)) / (s.cy + YSPACE);
	nCols = (rc.right - (2 * XMARGIN) - (5 * s.cx) - 17);
	nCols /= (3 * (s.cx + 1));
	if(nCols != 0)
	{
		nCols--;
	}
	
	if(pRows != NULL)
	{
		*pRows = nRows;
	}
	if(pCols != NULL)
	{
		*pCols = nCols;
	}
}

void CMemoryView::Paint(HDC hDC)
{
	RECT rwin;
	SIZE s;
	unsigned int nLines, nX, nY, i, j, nBytes, nFixedBytes, nOffset;
	uint32 nAddress;
	TCHAR sTemp[256];
	uint8 nValue;
	char sValue[2];
	HFONT hFont;

	GetClientRect(&rwin);

	BitBlt(hDC, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	hFont = GetFont();
	SelectObject(hDC, hFont);

	GetTextExtentPoint32(hDC, _T("0"), 1, &s);

	nLines = (rwin.bottom - (YMARGIN * 2)) / (s.cy + YSPACE);
	
	nFixedBytes = rwin.right - (2 * XMARGIN) - (5 * s.cx) - 17;
	nFixedBytes /= (3 * (s.cx + 1));
	if(nFixedBytes != 0)
	{
		nFixedBytes--;
	}
	//nFixedBytes &= ~(0x0F);
	nBytes = nFixedBytes;

	nLines++;

	nOffset = GetScrollOffset();

	nY = YMARGIN;
	nAddress = nOffset * nFixedBytes;
	for(i = 0; i < nLines; i++)
	{
		if(nAddress >= m_nSize)
		{
			break;
		}
		if((nAddress + nBytes) >= m_nSize)
		{
			nBytes = (m_nSize - nAddress);
		}

		nX = XMARGIN;
		_sntprintf(sTemp, countof(sTemp), _T("%0.8X"), nAddress);
		TextOut(hDC, nX, nY, sTemp, (int)_tcslen(sTemp));
		nX += (8 * s.cx) + 10;
		
		for(j = 0; j < nBytes; j++)
		{
			_sntprintf(sTemp, countof(sTemp), _T("%0.2X"), GetByte(nAddress + j));
			TextOut(hDC, nX, nY, sTemp, (int)_tcslen(sTemp));
			nX += (2 * s.cx) + 3;
		}
		nX += (nFixedBytes - nBytes) * (2 * s.cx + 3);

		nX += 10;

		for(j = 0; j < nBytes; j++)
		{
			nValue = GetByte(nAddress + j);
			if(nValue < 0x20 || nValue > 0x7F)
			{
				nValue = '.';
			}
			sValue[0] = nValue;
			sValue[1] = 0x00;
			TextOut(hDC, nX, nY, string_cast<tstring>(sValue).c_str(), 1);
			nX += s.cx;
		}

		nY += (s.cy + YSPACE);
		nAddress += nBytes;
	}

	DeleteObject(hFont);
}

void CMemoryView::SetMemorySize(uint32 nSize)
{
	m_nSize = nSize;
	UpdateScrollRange();
}

long CMemoryView::OnVScroll(unsigned int nType, unsigned int nTrackPos)
{
	SCROLLINFO si;
	int nOffset;
	nOffset = (int)GetScrollOffset();
	switch(nType)
	{
	case SB_LINEDOWN:
		nOffset++;
		break;
	case SB_LINEUP:
		nOffset--;
		break;
	case SB_PAGEDOWN:
		nOffset += 10;
		break;
	case SB_PAGEUP:
		nOffset -= 10;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nOffset = GetScrollThumbPosition();
		break;
	default:
		return FALSE;
		break;
	}

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= nOffset;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	Redraw();
	return TRUE;
}

long CMemoryView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	UpdateScrollRange();
	CCustomDrawn::OnSize(nType, nX, nY);
	return TRUE;
}

long CMemoryView::OnSetFocus()
{
	return FALSE;
}

long CMemoryView::OnMouseWheel(short nZ)
{
	if(nZ < 0)
	{
		OnVScroll(SB_LINEDOWN, 0);
	}
	else
	{
		OnVScroll(SB_LINEUP, 0);
	}
	return FALSE;
}

long CMemoryView::OnLeftButtonDown(int nX, int nY)
{
	SetFocus();
	return FALSE;
}
