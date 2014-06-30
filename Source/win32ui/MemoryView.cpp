#include <stdio.h>
#include <string.h>
#include "MemoryView.h"
#include "string_cast.h"
#include "win32/GdiObj.h"
#include "win32/ClientDeviceContext.h"
#include "win32/DefaultWndClass.h"
#include "lexical_cast_ex.h"

#define CLSNAME		_T("CMemoryView")
#define XMARGIN		5
#define YMARGIN		5
#define YSPACE		0

CMemoryView::CMemoryView(HWND parentWnd, const RECT& rect)
{
	Create(WS_EX_CLIENTEDGE, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, rect, parentWnd, NULL);
	SetClassPtr();

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
	unsigned int nRows = 0, nCols = 0;
	GetVisibleRowsCols(&nRows, &nCols);

	//Humm, nvm...
	if(nCols == 0) return;
	if(nAddress >= m_size) return;

	unsigned int nRow = (nAddress / nCols);

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= nRow;
	si.fMask		= SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	Redraw();
}

uint32 CMemoryView::GetSelection()
{
	return m_selectionStart;
}

void CMemoryView::UpdateScrollRange()
{
	unsigned int nRows = 0, nCols = 0;
	GetVisibleRowsCols(&nRows, &nCols);

	unsigned int nTotalRows = 0;
	if(nCols != 0)
	{
		nTotalRows = (m_size / nCols);
		if((m_size % nCols) != 0) nTotalRows++;
	}
	else
	{
		nTotalRows = 0;
	}

	SCROLLINFO si;
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
	SIZE s = {};

	{
		Framework::Win32::CClientDeviceContext deviceContext(m_hWnd);
		Framework::Win32::CFont font(GetFont());

		s = deviceContext.GetFontSize(font);
	}

	RECT rc = GetClientRect();

	unsigned int nRows = (rc.bottom - (YMARGIN * 2)) / (s.cy + YSPACE);
	unsigned int nCols = (rc.right - (2 * XMARGIN) - (5 * s.cx) - 17);
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
	Framework::Win32::CDeviceContext deviceContext(hDC);

	RECT rwin = GetClientRect();
	BitBlt(deviceContext, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	Framework::Win32::CFont font(GetFont());
	deviceContext.SelectObject(font);

	SIZE s = deviceContext.GetFontSize(font);

	unsigned int nLines = 0, nFixedBytes = 0;
	uint32 nAddress = 0;
	GetRenderParams(s, nLines, nFixedBytes, nAddress);

	unsigned int nBytes = nFixedBytes;
	unsigned int nY = YMARGIN;

	for(unsigned int i = 0; i < nLines; i++)
	{
		if(nAddress >= m_size)
		{
			break;
		}
		if((nAddress + nBytes) >= m_size)
		{
			nBytes = (m_size - nAddress);
		}

		unsigned int nX = XMARGIN;
		deviceContext.TextOut(nX, nY, lexical_cast_hex<std::tstring>(nAddress, 8).c_str());
		nX += (8 * s.cx) + 10;

		for(unsigned int j = 0; j < nBytes; j++)
		{
			deviceContext.TextOut(nX, nY, lexical_cast_hex<std::tstring>(GetByte(nAddress + j), 2).c_str());
			nX += (2 * s.cx) + 3;
		}
		nX += (nFixedBytes - nBytes) * (2 * s.cx + 3);

		nX += 10;

		for(unsigned int j = 0; j < nBytes; j++)
		{
			uint8 nValue;
			char sValue[2];

			nValue = GetByte(nAddress + j);
			if(nValue < 0x20 || nValue > 0x7F)
			{
				nValue = '.';
			}
			sValue[0] = nValue;
			sValue[1] = 0x00;

			deviceContext.TextOut(nX, nY, string_cast<std::tstring>(sValue).c_str());
			nX += s.cx;
		}

		nY += (s.cy + YSPACE);
		nAddress += nBytes;
	}
}

void CMemoryView::SetMemorySize(uint32 nSize)
{
	m_size = nSize;
	UpdateScrollRange();
}

long CMemoryView::OnVScroll(unsigned int nType, unsigned int nTrackPos)
{
	SCROLLINFO si;
	int nOffset = (int)GetScrollOffset();
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

	UpdateCaretPosition();

	Redraw();
	return TRUE;
}

long CMemoryView::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	UpdateScrollRange();
	UpdateCaretPosition();
	CCustomDrawn::OnSize(nType, nX, nY);
	return TRUE;
}

long CMemoryView::OnSetFocus()
{
	Framework::Win32::CClientDeviceContext deviceContext(m_hWnd);
	Framework::Win32::CFont font(GetFont());

	CreateCaret(m_hWnd, NULL, 2, deviceContext.GetFontHeight(font));
	ShowCaret(m_hWnd);

	UpdateCaretPosition();

	Redraw();

	return FALSE;
}

long CMemoryView::OnKillFocus()
{
	HideCaret(m_hWnd);
	DestroyCaret();

	return FALSE;
}

long CMemoryView::OnMouseWheel(int x, int y, short z)
{
	if(z < 0)
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

long CMemoryView::OnLeftButtonUp(int nX, int nY)
{
	SIZE fontSize = Framework::Win32::CClientDeviceContext(m_hWnd).GetFontSize(Framework::Win32::CFont(GetFont()));

	nY -= YMARGIN;
	nX -= XMARGIN + (8 * fontSize.cx) + 10;

	if(nY < 0) return FALSE;
	if(nX < 0) return FALSE;

	//Find selected line
	unsigned int nLine = nY / (fontSize.cy + YSPACE);

	//Find selected row;
	unsigned int nRow = nX / ((2 * fontSize.cx) + 3);

	uint32 nAddress = 0;
	unsigned int nCols = 0, nRows = 0;
	GetRenderParams(fontSize, nCols, nRows, nAddress);

	if(nRow >= nRows) return FALSE;

	SetSelectionStart(nAddress + nRow + (nLine * nRows));

	return FALSE;
}

long CMemoryView::OnKeyDown(WPARAM nKey, LPARAM)
{
	switch(nKey)
	{
	case VK_RIGHT:
		SetSelectionStart(m_selectionStart + 1);
		break;
	case VK_LEFT:
		SetSelectionStart(m_selectionStart - 1);
		break;
	case VK_UP:
	case VK_DOWN:
		{
			unsigned int nCols = 0;
			GetVisibleRowsCols(NULL, &nCols);
			SetSelectionStart(m_selectionStart + ((nKey == VK_UP) ? (-static_cast<int>(nCols)) : (nCols)));
		}
	}

	return TRUE;
}

void CMemoryView::SetSelectionStart(unsigned int nNewAddress)
{
	if(static_cast<int>(nNewAddress) < 0) return;
	if(nNewAddress >= m_size) return;

	m_selectionStart = nNewAddress;
	UpdateCaretPosition();
	OnSelectionChange(m_selectionStart);
}

void CMemoryView::UpdateCaretPosition()
{
	Framework::Win32::CClientDeviceContext deviceContext(m_hWnd);
	Framework::Win32::CFont font(GetFont());

	SIZE fontSize(deviceContext.GetFontSize(font));
	uint32 nAddress = 0;
	unsigned int nLines = 0, nBytes = 0;
	GetRenderParams(
		fontSize,
		nLines,
		nBytes,
		nAddress);

	if(
		(nBytes != 0) &&
		(m_selectionStart >= nAddress) && 
		(m_selectionStart <= (nAddress + (nLines * nBytes)))
		)
	{
		unsigned int nSelectionStart = m_selectionStart - nAddress;
		int nX = XMARGIN + (8 * fontSize.cx) + 10 + (nSelectionStart % nBytes) * ((2 * fontSize.cx) + 3);
		int nY = YMARGIN + (fontSize.cy + YSPACE) * (nSelectionStart / nBytes);
		SetCaretPos(nX, nY);
	}
	else
	{
		SetCaretPos(-20, -20);
	}
}

void CMemoryView::GetRenderParams(const SIZE& FontSize, unsigned int& nLines, unsigned int& nFixedBytes, uint32& nAddress)
{
	RECT clientRect(GetClientRect());

	nLines = (clientRect.bottom - (YMARGIN * 2)) / (FontSize.cy + YSPACE);
	
	nFixedBytes = clientRect.right - (2 * XMARGIN) - (5 * FontSize.cx) - 17;
	nFixedBytes /= (3 * (FontSize.cx + 1));
	if(nFixedBytes != 0)
	{
		nFixedBytes--;
	}
	//nFixedBytes &= ~(0x0F);
	//nBytes = nFixedBytes;

	nLines++;

	nAddress = GetScrollOffset() * nFixedBytes;
}
