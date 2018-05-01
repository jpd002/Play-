#include <stdio.h>
#include <string.h>
#include "win32/ClientDeviceContext.h"
#include "win32/DefaultWndClass.h"
#include "win32/Font.h"
#include "win32/DpiUtils.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"
#include "MemoryView.h"

#define ADDRESSCHARS 8
#define PAGESIZE 10

const CMemoryView::UNITINFO CMemoryView::g_units[] =
    {
        {1, 2, &CMemoryView::RenderByteUnit, _T("8-bit Integers")},
        {4, 8, &CMemoryView::RenderWordUnit, _T("32-bit Integers")},
        {4, 11, &CMemoryView::RenderSingleUnit, _T("Single Precision Floating Point Numbers")}};

CMemoryView::CMemoryView(HWND parentWnd, const RECT& rect)
    : m_font(reinterpret_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT)))
{
	static_assert(countof(CMemoryView::g_units) <= CMemoryView::MAX_UNITS, "Too many units are defined.");

	{
		m_renderMetrics.xmargin = Framework::Win32::PointsToPixels(5);
		m_renderMetrics.ymargin = Framework::Win32::PointsToPixels(5);
		m_renderMetrics.yspace = Framework::Win32::PointsToPixels(0);
		m_renderMetrics.unitSpacing = Framework::Win32::PointsToPixels(3);
		m_renderMetrics.lineSectionSpacing = Framework::Win32::PointsToPixels(10);
	}

	Create(WS_EX_CLIENTEDGE, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_VISIBLE | WS_VSCROLL | WS_CHILD, rect, parentWnd, NULL);
	SetClassPtr();

	UpdateScrollRange();
}

uint32 CMemoryView::GetSelection()
{
	return m_selectionStart;
}

void CMemoryView::SetSelectionStart(unsigned int newAddress)
{
	newAddress = std::max<int>(newAddress, 0);
	newAddress = std::min<int>(newAddress, m_size - 1);

	m_selectionStart = newAddress;
	EnsureSelectionVisible();
	UpdateCaretPosition();
	OnSelectionChange(m_selectionStart);
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

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.nMin = 0;
	if(totalLines <= renderParams.totallyVisibleLines)
	{
		si.nMax = 0;
	}
	else
	{
		si.nMax = totalLines - renderParams.totallyVisibleLines;
	}
	si.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
}

unsigned int CMemoryView::GetScrollOffset()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nPos;
}

unsigned int CMemoryView::GetScrollThumbPosition()
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_TRACKPOS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	return si.nTrackPos;
}

HMENU CMemoryView::CreateContextualMenu()
{
	HMENU menu = CreatePopupMenu();

	{
		HMENU bytesPerLineMenu = CreatePopupMenu();

		AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 0) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_AUTO, _T("Auto"));
		AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 16) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_16BYTES, _T("16 bytes"));
		AppendMenu(bytesPerLineMenu, MF_STRING | ((m_bytesPerLine == 32) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_COLUMNS_32BYTES, _T("32 bytes"));

		AppendMenu(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(bytesPerLineMenu), _T("Bytes Per Line"));
	}

	{
		HMENU unitMenu = CreatePopupMenu();

		for(uint32 i = 0; i < countof(g_units); i++)
		{
			const auto& unit = g_units[i];
			AppendMenu(unitMenu, MF_STRING | ((i == m_selectedUnit) ? MF_CHECKED : MF_UNCHECKED), ID_MEMORYVIEW_UNIT_BASE + i, unit.description);
		}

		AppendMenu(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(unitMenu), _T("Display Unit"));
	}

	return menu;
}

void CMemoryView::Paint(HDC hDC)
{
	Framework::Win32::CDeviceContext deviceContext(hDC);

	auto rwin = GetClientRect();
	BitBlt(deviceContext, 0, 0, rwin.Right(), rwin.Bottom(), NULL, 0, 0, WHITENESS);

	deviceContext.SelectObject(m_font);

	auto fontSize = Framework::Win32::GetFixedFontSize(m_font);

	auto renderParams = GetRenderParams();
	unsigned int y = m_renderMetrics.ymargin;
	uint32 address = renderParams.address;

	for(unsigned int i = 0; i < renderParams.lines; i++)
	{
		if(address >= m_size)
		{
			break;
		}

		unsigned int bytesForCurrentLine = renderParams.bytesPerLine;
		unsigned int unitsPerLine = renderParams.bytesPerLine / renderParams.unit->bytesPerUnit;
		if((address + bytesForCurrentLine) >= m_size)
		{
			bytesForCurrentLine = (m_size - address);
		}

		unsigned int x = m_renderMetrics.xmargin;
		deviceContext.TextOut(x, y, lexical_cast_hex<std::tstring>(address, ADDRESSCHARS).c_str());
		x += (ADDRESSCHARS * fontSize.cx) + m_renderMetrics.lineSectionSpacing;

		unsigned int unitsForCurrentLine = bytesForCurrentLine / renderParams.unit->bytesPerUnit;
		for(unsigned int j = 0; j < unitsForCurrentLine; j++)
		{
			auto unitAddress = address + (j * renderParams.unit->bytesPerUnit);
			auto outputString = (this->*(renderParams.unit->renderer))(unitAddress);
			deviceContext.TextOut(x, y, outputString.c_str());
			x += (renderParams.unit->charsPerUnit * fontSize.cx) + m_renderMetrics.unitSpacing;
		}
		//Compensate for incomplete lines (when bytesForCurrentLine < bytesPerLine)
		x += (unitsPerLine - unitsForCurrentLine) * (renderParams.unit->charsPerUnit * fontSize.cx + m_renderMetrics.unitSpacing);

		x += m_renderMetrics.lineSectionSpacing;

		for(unsigned int j = 0; j < bytesForCurrentLine; j++)
		{
			uint8 value = GetByte(address + j);
			if(value < 0x20 || value > 0x7F)
			{
				value = '.';
			}

			char valueString[2];
			valueString[0] = value;
			valueString[1] = 0x00;

			deviceContext.TextOut(x, y, string_cast<std::tstring>(valueString).c_str());
			x += fontSize.cx;
		}

		y += (fontSize.cy + m_renderMetrics.yspace);
		address += bytesForCurrentLine;
	}
}

void CMemoryView::SetMemorySize(uint32 size)
{
	m_size = size;
	UpdateScrollRange();
}

uint32 CMemoryView::GetBytesPerLine() const
{
	return m_bytesPerLine;
}

void CMemoryView::SetBytesPerLine(uint32 bytesPerLine)
{
	m_bytesPerLine = bytesPerLine;
	UpdateScrollRange();
	EnsureSelectionVisible();
	UpdateCaretPosition();
	Redraw();
}

void CMemoryView::SetDisplayUnit(uint32 displayUnit)
{
	const auto& unit = g_units[displayUnit];
	assert(displayUnit < countof(g_units));
	m_selectedUnit = displayUnit;
	SetSelectionStart(m_selectionStart & ~(unit.bytesPerUnit - 1));
	UpdateScrollRange();
	EnsureSelectionVisible();
	UpdateCaretPosition();
	Redraw();
}

long CMemoryView::OnVScroll(unsigned int scrollType, unsigned int)
{
	SCROLLINFO si;
	int offset = (int)GetScrollOffset();
	switch(scrollType)
	{
	case SB_LINEDOWN:
		offset++;
		break;
	case SB_LINEUP:
		offset--;
		break;
	case SB_PAGEDOWN:
		offset += PAGESIZE;
		break;
	case SB_PAGEUP:
		offset -= PAGESIZE;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		offset = GetScrollThumbPosition();
		break;
	default:
		return FALSE;
		break;
	}

	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.nPos = offset;
	si.fMask = SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	UpdateCaretPosition();

	Redraw();
	return TRUE;
}

long CMemoryView::OnCommand(unsigned short id, unsigned short, HWND)
{
	switch(id)
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
	if((id >= ID_MEMORYVIEW_UNIT_BASE) && (id < ID_MEMORYVIEW_UNIT_MAX))
	{
		SetDisplayUnit(id - ID_MEMORYVIEW_UNIT_BASE);
	}
	return TRUE;
}

long CMemoryView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	UpdateScrollRange();
	UpdateCaretPosition();
	CCustomDrawn::OnSize(type, x, y);
	return TRUE;
}

long CMemoryView::OnSetFocus()
{
	auto fontSize = Framework::Win32::GetFixedFontSize(m_font);

	CreateCaret(m_hWnd, NULL, 2, fontSize.cy);
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

long CMemoryView::OnLeftButtonDown(int x, int y)
{
	SetFocus();
	return FALSE;
}

long CMemoryView::OnLeftButtonUp(int x, int y)
{
	auto fontSize = Framework::Win32::GetFixedFontSize(m_font);

	y -= m_renderMetrics.ymargin;
	x -= m_renderMetrics.xmargin + (ADDRESSCHARS * fontSize.cx) + m_renderMetrics.lineSectionSpacing;

	if(y < 0) return FALSE;
	if(x < 0) return FALSE;

	auto renderParams = GetRenderParams();

	unsigned int selectedLine = y / (fontSize.cy + m_renderMetrics.yspace);
	unsigned int selectedLineUnit = x / ((renderParams.unit->charsPerUnit * fontSize.cx) + m_renderMetrics.unitSpacing);
	unsigned int selectedLineByte = selectedLineUnit * renderParams.unit->bytesPerUnit;

	if(selectedLineByte >= renderParams.bytesPerLine) return FALSE;

	SetSelectionStart(renderParams.address + selectedLineByte + (selectedLine * renderParams.bytesPerLine));

	return FALSE;
}

long CMemoryView::OnRightButtonUp(int x, int y)
{
	HMENU contextualMenu = CreateContextualMenu();

	POINT pt;
	pt.x = x;
	pt.y = y;
	ClientToScreen(m_hWnd, &pt);

	TrackPopupMenu(contextualMenu, 0, pt.x, pt.y, 0, m_hWnd, NULL);

	return FALSE;
}

long CMemoryView::OnKeyDown(WPARAM key, LPARAM)
{
	switch(key)
	{
	case VK_RIGHT:
	{
		auto renderParams = GetRenderParams();
		SetSelectionStart(m_selectionStart + renderParams.unit->bytesPerUnit);
	}
	break;
	case VK_LEFT:
	{
		auto renderParams = GetRenderParams();
		SetSelectionStart(m_selectionStart - renderParams.unit->bytesPerUnit);
	}
	break;
	case VK_UP:
	case VK_DOWN:
	{
		auto renderParams = GetRenderParams();
		SetSelectionStart(m_selectionStart + ((key == VK_UP) ? (-static_cast<int>(renderParams.bytesPerLine)) : (renderParams.bytesPerLine)));
	}
	break;
	case VK_PRIOR:
	case VK_NEXT:
	{
		auto renderParams = GetRenderParams();
		SetSelectionStart(m_selectionStart + ((key == VK_PRIOR) ? (-static_cast<int>(renderParams.bytesPerLine * PAGESIZE)) : (renderParams.bytesPerLine * PAGESIZE)));
	}
	break;
	}

	return TRUE;
}

void CMemoryView::UpdateCaretPosition()
{
	auto fontSize = Framework::Win32::GetFixedFontSize(m_font);
	auto renderParams = GetRenderParams();
	if(
	    (renderParams.bytesPerLine != 0) &&
	    (m_selectionStart >= renderParams.address) &&
	    (m_selectionStart <= (renderParams.address + (renderParams.lines * renderParams.bytesPerLine))))
	{
		uint32 selectionStart = m_selectionStart - renderParams.address;
		assert((selectionStart % renderParams.unit->bytesPerUnit) == 0);
		uint32 selectionStartUnit = (selectionStart % renderParams.bytesPerLine) / renderParams.unit->bytesPerUnit;
		int x = m_renderMetrics.xmargin + (ADDRESSCHARS * fontSize.cx) + m_renderMetrics.lineSectionSpacing + selectionStartUnit * ((renderParams.unit->charsPerUnit * fontSize.cx) + m_renderMetrics.unitSpacing);
		int y = m_renderMetrics.ymargin + (fontSize.cy + m_renderMetrics.yspace) * (selectionStart / renderParams.bytesPerLine);
		SetCaretPos(x, y);
	}
	else
	{
		SetCaretPos(-20, -20);
	}
}

void CMemoryView::EnsureSelectionVisible()
{
	auto renderParams = GetRenderParams();
	uint32 address = m_selectionStart;

	if(renderParams.bytesPerLine == 0) return;
	if(address >= m_size) return;

	uint32 visibleMin = renderParams.address;
	uint32 visibleMax = renderParams.address + (renderParams.bytesPerLine * renderParams.totallyVisibleLines);

	//Already visible
	if(address >= visibleMin && address < visibleMax)
	{
		return;
	}

	unsigned int line = (address / renderParams.bytesPerLine);

	SCROLLINFO si;
	memset(&si, 0, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.nPos = line;
	si.fMask = SIF_POS;
	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	Redraw();
}

CMemoryView::RENDERPARAMS CMemoryView::GetRenderParams()
{
	auto fontSize = Framework::Win32::GetFixedFontSize(m_font);

	RENDERPARAMS renderParams;
	assert(m_selectedUnit < countof(g_units));
	renderParams.unit = &g_units[m_selectedUnit];

	RECT clientRect(GetClientRect());

	renderParams.totallyVisibleLines = (clientRect.bottom - (m_renderMetrics.ymargin * 2)) / (fontSize.cy + m_renderMetrics.yspace);
	renderParams.lines = renderParams.totallyVisibleLines + 1;

	if(m_bytesPerLine == 0)
	{
		//lineSize =
		//    (2 * m_renderMetrics.xmargin)                                  Two Margins
		//  + (2 * m_renderMetrics.lineSectionSpacing)                       Two Spacings (between address and units, units and chars)
		//  + (ADDRESSCHARS * cx)                                            Address
		//  + unitsPerLine * (UNITCHARS * cx + m_renderMetrics.unitSpacing)  Units
		//  + unitsPerLine * UNITBYTES * cx                                  Chars
		unsigned int unitsPerLine = clientRect.right - (2 * m_renderMetrics.xmargin) - (2 * m_renderMetrics.lineSectionSpacing) - (ADDRESSCHARS * fontSize.cx);
		unitsPerLine /= ((renderParams.unit->charsPerUnit * fontSize.cx + m_renderMetrics.unitSpacing) + (renderParams.unit->bytesPerUnit * fontSize.cx));
		renderParams.bytesPerLine = unitsPerLine * renderParams.unit->bytesPerUnit;
	}
	else
	{
		renderParams.bytesPerLine = m_bytesPerLine;
	}

	renderParams.address = GetScrollOffset() * renderParams.bytesPerLine;

	return renderParams;
}

std::tstring CMemoryView::RenderByteUnit(uint32 address)
{
	uint8 unitValue = GetByte(address);
	return string_format(_T("%02X"), unitValue);
}

std::tstring CMemoryView::RenderWordUnit(uint32 address)
{
	uint32 unitValue =
	    (GetByte(address + 0) << 0) |
	    (GetByte(address + 1) << 8) |
	    (GetByte(address + 2) << 16) |
	    (GetByte(address + 3) << 24);
	return string_format(_T("%08X"), unitValue);
}

std::tstring CMemoryView::RenderSingleUnit(uint32 address)
{
	uint32 unitValue =
	    (GetByte(address + 0) << 0) |
	    (GetByte(address + 1) << 8) |
	    (GetByte(address + 2) << 16) |
	    (GetByte(address + 3) << 24);
	return string_format(_T("%+04.4e"), *reinterpret_cast<const float*>(&unitValue));
}
