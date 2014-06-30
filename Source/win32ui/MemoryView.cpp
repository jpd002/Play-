#include <stdio.h>
#include <string.h>
#include "MemoryView.h"
#include "string_cast.h"
#include "win32/ClientDeviceContext.h"
#include "win32/DefaultWndClass.h"
#include "lexical_cast_ex.h"

#define XMARGIN				5
#define YMARGIN				5
#define YSPACE				0
#define BYTESPACING			3
#define LINESECTIONSPACING	10
#define ADDRESSCHARS		8

CMemoryView::CMemoryView(HWND parentWnd, const RECT& rect)
: m_font(reinterpret_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT)))
{
	Create(WS_EX_CLIENTEDGE, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, rect, parentWnd, NULL);
	SetClassPtr();

	UpdateScrollRange();
}

CMemoryView::~CMemoryView()
{

}

void CMemoryView::ScrollToAddress(uint32 address)
{
	auto renderParams = GetRenderParams();

	//Humm, nvm...
	if(renderParams.bytesPerLine == 0) return;
	if(address >= m_size) return;

	unsigned int line = (address / renderParams.bytesPerLine);

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nPos			= line;
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
	auto renderParams = GetRenderParams();

	unsigned int totalLines = 0;
	if(renderParams.bytesPerLine != 0)
	{
		totalLines = (m_size / renderParams.bytesPerLine);
		if((m_size % renderParams.bytesPerLine) != 0) totalLines++;
	}
	else
	{
		totalLines = 0;
	}

	//Render params always compensate for partially visible lines
	unsigned int totallyVisibleLines = renderParams.lines - 1;

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize		= sizeof(SCROLLINFO);
	si.nMin			= 0;
	if(totalLines <= totallyVisibleLines)
	{
		si.nMax		= 0;
	}
	else
	{
		si.nMax		= totalLines - totallyVisibleLines;
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

HMENU CMemoryView::CreateContextualMenu()
{
	HMENU bytesPerLineMenu = CreatePopupMenu();
	AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 0) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_AUTO,		_T("Auto"));
	AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 16) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_16BYTES,	_T("16 bytes"));
	AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 32) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_32BYTES,	_T("32 bytes"));

	HMENU menu = CreatePopupMenu();
	AppendMenu(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(bytesPerLineMenu), _T("Bytes Per Line"));

	return menu;
}

void CMemoryView::Paint(HDC hDC)
{
	Framework::Win32::CDeviceContext deviceContext(hDC);

	RECT rwin = GetClientRect();
	BitBlt(deviceContext, 0, 0, rwin.right, rwin.bottom, NULL, 0, 0, WHITENESS);

	deviceContext.SelectObject(m_font);

	SIZE fontSize = deviceContext.GetFontSize(m_font);

	auto renderParams = GetRenderParams();
	unsigned int nY = YMARGIN;
	uint32 address = renderParams.address;

	for(unsigned int i = 0; i < renderParams.lines; i++)
	{
		if(address >= m_size)
		{
			break;
		}

		unsigned int bytesForCurrentLine = renderParams.bytesPerLine;
		if((address + bytesForCurrentLine) >= m_size)
		{
			bytesForCurrentLine = (m_size - address);
		}

		unsigned int nX = XMARGIN;
		deviceContext.TextOut(nX, nY, lexical_cast_hex<std::tstring>(address, ADDRESSCHARS).c_str());
		nX += (ADDRESSCHARS * fontSize.cx) + LINESECTIONSPACING;

		for(unsigned int j = 0; j < bytesForCurrentLine; j++)
		{
			deviceContext.TextOut(nX, nY, lexical_cast_hex<std::tstring>(GetByte(address + j), 2).c_str());
			nX += (2 * fontSize.cx) + BYTESPACING;
		}
		//Compensate for incomplete lines (when bytesForCurrentLine < bytesPerLine)
		nX += (renderParams.bytesPerLine - bytesForCurrentLine) * (2 * fontSize.cx + BYTESPACING);

		nX += LINESECTIONSPACING;

		for(unsigned int j = 0; j < bytesForCurrentLine; j++)
		{
			uint8 nValue = GetByte(address + j);
			if(nValue < 0x20 || nValue > 0x7F)
			{
				nValue = '.';
			}

			char sValue[2];
			sValue[0] = nValue;
			sValue[1] = 0x00;

			deviceContext.TextOut(nX, nY, string_cast<std::tstring>(sValue).c_str());
			nX += fontSize.cx;
		}

		nY += (fontSize.cy + YSPACE);
		address += bytesForCurrentLine;
	}
}

void CMemoryView::SetMemorySize(uint32 size)
{
	m_size = size;
	UpdateScrollRange();
}

void CMemoryView::SetBytesPerLine(uint32 bytesPerLine)
{
	m_bytesPerLine = bytesPerLine;
	UpdateScrollRange();
	ScrollToAddress(m_selectionStart);
	UpdateCaretPosition();
	Redraw();
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

long CMemoryView::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	switch(nID)
	{
	case ID_MEMORYVIEW_COLUMNS_AUTO:
		SetBytesPerLine(0);
		break;
	case ID_MEMORYVIEW_COLUMNS_16BYTES:
		SetBytesPerLine(0x10);
		break;
	case ID_MEMORYVIEW_COLUMNS_32BYTES:
		SetBytesPerLine(0x20);
		break;
	}
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

	CreateCaret(m_hWnd, NULL, 2, deviceContext.GetFontHeight(m_font));
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
	SIZE fontSize = Framework::Win32::CClientDeviceContext(m_hWnd).GetFontSize(m_font);

	nY -= YMARGIN;
	nX -= XMARGIN + (ADDRESSCHARS * fontSize.cx) + LINESECTIONSPACING;

	if(nY < 0) return FALSE;
	if(nX < 0) return FALSE;

	unsigned int selectedLine = nY / (fontSize.cy + YSPACE);
	unsigned int selectedByte = nX / ((2 * fontSize.cx) + BYTESPACING);

	auto renderParams = GetRenderParams();

	if(selectedByte >= renderParams.bytesPerLine) return FALSE;

	SetSelectionStart(renderParams.address + selectedByte + (selectedLine * renderParams.bytesPerLine));

	return FALSE;
}

long CMemoryView::OnRightButtonUp(int nX, int nY)
{
	HMENU contextualMenu = CreateContextualMenu();

	POINT pt;
	pt.x = nX;
	pt.y = nY;
	ClientToScreen(m_hWnd, &pt);

	TrackPopupMenu(contextualMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL); 

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
			auto renderParams = GetRenderParams();
			SetSelectionStart(m_selectionStart + ((nKey == VK_UP) ? (-static_cast<int>(renderParams.bytesPerLine)) : (renderParams.bytesPerLine)));
		}
		break;
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
	SIZE fontSize(deviceContext.GetFontSize(m_font));
	auto renderParams = GetRenderParams();
	if(
		(renderParams.bytesPerLine != 0) &&
		(m_selectionStart >= renderParams.address) && 
		(m_selectionStart <= (renderParams.address + (renderParams.lines * renderParams.bytesPerLine)))
		)
	{
		unsigned int selectionStart = m_selectionStart - renderParams.address;
		int nX = XMARGIN + (ADDRESSCHARS * fontSize.cx) + LINESECTIONSPACING + (selectionStart % renderParams.bytesPerLine) * ((2 * fontSize.cx) + BYTESPACING);
		int nY = YMARGIN + (fontSize.cy + YSPACE) * (selectionStart / renderParams.bytesPerLine);
		SetCaretPos(nX, nY);
	}
	else
	{
		SetCaretPos(-20, -20);
	}
}

CMemoryView::RENDERPARAMS CMemoryView::GetRenderParams()
{
	Framework::Win32::CClientDeviceContext deviceContext(m_hWnd);
	SIZE fontSize(deviceContext.GetFontSize(m_font));

	RENDERPARAMS renderParams;

	RECT clientRect(GetClientRect());

	renderParams.lines = (clientRect.bottom - (YMARGIN * 2)) / (fontSize.cy + YSPACE);
	renderParams.lines++;
	
	if(m_bytesPerLine == 0)
	{
		//lineSize = (2 * XMARGIN) + (2 * LINESECTIONSPACING) + (ADDRESSCHARS * cx) + bytesPerLine * (2 * cx + BYTESPACING) + bytesPerLine * cx
		renderParams.bytesPerLine = clientRect.right - (2 * XMARGIN) - (2 * LINESECTIONSPACING) - (ADDRESSCHARS * fontSize.cx);
		renderParams.bytesPerLine /= ((2 * fontSize.cx + BYTESPACING) + (fontSize.cx));
	}
	else
	{
		renderParams.bytesPerLine = m_bytesPerLine;
	}

	renderParams.address = GetScrollOffset() * renderParams.bytesPerLine;

	return renderParams;
}
