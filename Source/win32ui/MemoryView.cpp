#include <stdio.h>
#include <string.h>
#include "MemoryView.h"
#include "string_cast.h"
#include "win32/GdiObj.h"
#include "win32/ClientDeviceContext.h"
#include "lexical_cast_ex.h"

#define CLSNAME		_T("CMemoryView")
#define XMARGIN		5
#define YMARGIN		5
#define YSPACE		0

using namespace std;
using namespace Framework;

CMemoryView::CMemoryView(HWND hParent, const RECT& rect)
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

	Create(WS_EX_CLIENTEDGE, CLSNAME, _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, rect, hParent, NULL);
	SetClassPtr();

	m_nSize = 0;
	m_nSelectionStart = 0;

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

uint32 CMemoryView::GetSelection()
{
	return m_nSelectionStart;
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
	SIZE s;
	unsigned int nRows, nCols;

	{
		Win32::CClientDeviceContext DeviceContext(m_hWnd);
		Win32::CFont Font(GetFont());

		s = DeviceContext.GetFontSize(Font);
	}

	RECT rc = GetClientRect();

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
	unsigned int nLines, nX, nY, nBytes, nFixedBytes;
	uint32 nAddress;
	Win32::CDeviceContext DeviceContext(hDC);

	RECT rwin = GetClientRect();
	BitBlt(DeviceContext, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	Win32::CFont Font(GetFont());
	DeviceContext.SelectObject(Font);

	SIZE s = DeviceContext.GetFontSize(Font);

	GetRenderParams(s, nLines, nFixedBytes, nAddress);

	nBytes = nFixedBytes;
	nY = YMARGIN;

	for(unsigned int i = 0; i < nLines; i++)
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
		DeviceContext.TextOut(nX, nY, lexical_cast_hex<tstring>(nAddress, 8).c_str());
		nX += (8 * s.cx) + 10;

		for(unsigned int j = 0; j < nBytes; j++)
		{
			DeviceContext.TextOut(nX, nY, lexical_cast_hex<tstring>(GetByte(nAddress + j), 2).c_str());
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

			DeviceContext.TextOut(nX, nY, string_cast<tstring>(sValue).c_str());
			nX += s.cx;
		}

		nY += (s.cy + YSPACE);
		nAddress += nBytes;
	}
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
    Win32::CClientDeviceContext DeviceContext(m_hWnd);
    Win32::CFont Font(GetFont());

    CreateCaret(m_hWnd, NULL, 2, DeviceContext.GetFontHeight(Font));
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

long CMemoryView::OnLeftButtonUp(int nX, int nY)
{
    SIZE FontSize = Win32::CClientDeviceContext(m_hWnd).GetFontSize(Win32::CFont(GetFont()));

    nY -= YMARGIN;
    nX -= XMARGIN + (8 * FontSize.cx) + 10;

    if(nY < 0) return FALSE;
    if(nX < 0) return FALSE;

    unsigned int nLine, nRow;
    uint32 nAddress;
    unsigned int nCols, nRows;

    //Find selected line
    nLine = nY / (FontSize.cy + YSPACE);

    //Find selected row;
    nRow = nX / ((2 * FontSize.cx) + 3);
   
    GetRenderParams(FontSize, nCols, nRows, nAddress);

    if(nRow >= nRows) return FALSE;

    SetSelectionStart(nAddress + nRow + (nLine * nRows));

    return FALSE;
}

long CMemoryView::OnKeyDown(unsigned int nKey)
{
    switch(nKey)
    {
    case VK_RIGHT:
        SetSelectionStart(m_nSelectionStart + 1);
        break;
    case VK_LEFT:
        SetSelectionStart(m_nSelectionStart - 1);
        break;
    case VK_UP:
    case VK_DOWN:
        {
            unsigned int nCols;
            GetVisibleRowsCols(NULL, &nCols);
            SetSelectionStart(m_nSelectionStart + ((nKey == VK_UP) ? (-static_cast<int>(nCols)) : (nCols)));
        }
    }

    return TRUE;
}

void CMemoryView::SetSelectionStart(unsigned int nNewAddress)
{
	if(static_cast<int>(nNewAddress) < 0) return;
	if(nNewAddress >= m_nSize) return;

	m_nSelectionStart = nNewAddress;
	UpdateCaretPosition();
	OnSelectionChange(m_nSelectionStart);
}

void CMemoryView::UpdateCaretPosition()
{
	Win32::CClientDeviceContext DeviceContext(m_hWnd);
	Win32::CFont Font(GetFont());

	unsigned int nLines, nBytes;
	uint32 nAddress;
	SIZE FontSize(DeviceContext.GetFontSize(Font));

	GetRenderParams(
		FontSize,
		nLines,
		nBytes,
		nAddress);

	if(
		(nBytes != 0) &&
		(m_nSelectionStart >= nAddress) && 
		(m_nSelectionStart <= (nAddress + (nLines * nBytes)))
		)
	{
		int nX, nY;
		unsigned int nSelectionStart = m_nSelectionStart - nAddress;

		nX = XMARGIN + (8 * FontSize.cx) + 10 + (nSelectionStart % nBytes) * ((2 * FontSize.cx) + 3);
		nY = YMARGIN + (FontSize.cy + YSPACE) * (nSelectionStart / nBytes);
		SetCaretPos(nX, nY);
	}
	else
	{
		SetCaretPos(-20, -20);
	}
}

void CMemoryView::GetRenderParams(const SIZE& FontSize, unsigned int& nLines, unsigned int& nFixedBytes, uint32& nAddress)
{
	RECT ClientRect(GetClientRect());

	nLines = (ClientRect.bottom - (YMARGIN * 2)) / (FontSize.cy + YSPACE);
	
	nFixedBytes = ClientRect.right - (2 * XMARGIN) - (5 * FontSize.cx) - 17;
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
