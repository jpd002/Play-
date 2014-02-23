#include "SpuRegView.h"
#include "win32/Rect.h"

#define CANVAS_WIDTH		(470)
#define CANVAS_HEIGHT		(350)

CSpuRegView::CSpuRegView(HWND parentWnd, const TCHAR* title) 
: CDirectXControl(parentWnd, WS_VSCROLL | WS_HSCROLL)
, m_spu(nullptr)
, m_title(title)
, m_offsetX(0)
, m_offsetY(0)
, m_pageSizeX(0)
, m_pageSizeY(0)
, m_maxScrollX(0)
, m_maxScrollY(0)
{
	CreateResources();
	InitializeScrollBars();
	SetTimer(m_hWnd, NULL, 16, NULL);
}

CSpuRegView::~CSpuRegView()
{

}

long CSpuRegView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	InitializeScrollBars();
	return CDirectXControl::OnSize(type, x, y);
}

long CSpuRegView::OnTimer(WPARAM param)
{
	if(IsWindowVisible(m_hWnd))
	{
		Refresh();
	}
	return TRUE;
}

long CSpuRegView::OnHScroll(unsigned int type, unsigned int position)
{
	switch(type)
	{
	case SB_LINEDOWN:
		m_offsetX += 1;
		break;
	case SB_LINEUP:
		m_offsetX -= 1;
		break;
	case SB_PAGEDOWN:
		m_offsetX += m_pageSizeX;
		break;
	case SB_PAGEUP:
		m_offsetX -= m_pageSizeX;
		break;
	case SB_THUMBTRACK:
		m_offsetX = position;
		break;
	case SB_THUMBPOSITION:
		m_offsetX = position;
		break;
	}

	if(m_offsetX < 0)				m_offsetX = 0;
	if(m_offsetX > m_maxScrollX)	m_offsetX = m_maxScrollX;

	UpdateHorizontalScroll();

	return FALSE;
}

long CSpuRegView::OnVScroll(unsigned int type, unsigned int position)
{
	switch(type)
	{
	case SB_LINEDOWN:
		m_offsetY += 1;
		break;
	case SB_LINEUP:
		m_offsetY -= 1;
		break;
	case SB_PAGEDOWN:
		m_offsetY += m_pageSizeY;
		break;
	case SB_PAGEUP:
		m_offsetY -= m_pageSizeY;
		break;
	case SB_THUMBTRACK:
		m_offsetY = position;
		break;
	case SB_THUMBPOSITION:
		m_offsetY = position;
		break;
	}

	if(m_offsetY < 0)				m_offsetY = 0;
	if(m_offsetY > m_maxScrollY)	m_offsetY = m_maxScrollY;

	UpdateVerticalScroll();

	return FALSE;
}

long CSpuRegView::OnMouseWheel(int, int, short delta)
{
	m_offsetY -= delta / 40;
	m_offsetY = std::max<int>(m_offsetY, 0);
	m_offsetY = std::min<int>(m_offsetY, m_maxScrollY);
	UpdateVerticalScroll();
	return TRUE;
}

long CSpuRegView::OnKeyDown(unsigned int keyId)
{
	if(keyId == VK_LEFT || keyId == VK_RIGHT)
	{
		m_offsetX += (keyId == VK_LEFT ? -5 : 5);
		m_offsetX = std::max<int>(m_offsetX, 0);
		m_offsetX = std::min<int>(m_offsetX, m_maxScrollX);
		UpdateHorizontalScroll();
	}
	if(keyId == VK_UP || keyId == VK_DOWN)
	{
		m_offsetY += (keyId == VK_UP ? -5 : 5);
		m_offsetY = std::max<int>(m_offsetY, 0);
		m_offsetY = std::min<int>(m_offsetY, m_maxScrollY);
		UpdateVerticalScroll();
	}
	return TRUE;
}

long CSpuRegView::OnGetDlgCode(WPARAM, LPARAM)
{
	return DLGC_WANTARROWS;
}

void CSpuRegView::CreateResources()
{
	if(m_device.IsEmpty()) return;
	D3DXCreateFont(m_device, -11, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"), &m_font);
}

void CSpuRegView::InitializeScrollBars()
{
	RECT clientRect = GetClientRect();

	m_offsetX = 0;
	m_offsetY = 0;

	m_pageSizeX = clientRect.right;
	m_maxScrollX = CANVAS_WIDTH - (m_pageSizeX - 1);

	m_pageSizeY = clientRect.bottom;
	m_maxScrollY = CANVAS_HEIGHT - (m_pageSizeY - 1);

	{
		SCROLLINFO scrollInfo;
		memset(&scrollInfo, 0, sizeof(SCROLLINFO));
		scrollInfo.cbSize		= sizeof(SCROLLINFO);
		scrollInfo.fMask		= SIF_POS | SIF_RANGE | SIF_PAGE;
		scrollInfo.nMin			= 0;
		scrollInfo.nMax			= CANVAS_WIDTH;
		scrollInfo.nPos			= m_offsetX;
		scrollInfo.nPage		= m_pageSizeX;
		SetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo, TRUE);
	}

	{
		SCROLLINFO scrollInfo;
		memset(&scrollInfo, 0, sizeof(SCROLLINFO));
		scrollInfo.cbSize		= sizeof(SCROLLINFO);
		scrollInfo.fMask		= SIF_POS | SIF_RANGE | SIF_PAGE;
		scrollInfo.nMin			= 0;
		scrollInfo.nMax			= CANVAS_HEIGHT;
		scrollInfo.nPos			= m_offsetY;
		scrollInfo.nPage		= m_pageSizeY;
		SetScrollInfo(m_hWnd, SB_VERT, &scrollInfo, TRUE);
	}
}

void CSpuRegView::UpdateHorizontalScroll()
{
	SCROLLINFO scrollInfo;
	memset(&scrollInfo, 0, sizeof(SCROLLINFO));
	scrollInfo.cbSize		= sizeof(SCROLLINFO);
	scrollInfo.fMask		= SIF_POS;
	scrollInfo.nPos			= m_offsetX;
	SetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo, TRUE);
}

void CSpuRegView::UpdateVerticalScroll()
{
	SCROLLINFO scrollInfo;
	memset(&scrollInfo, 0, sizeof(SCROLLINFO));
	scrollInfo.cbSize		= sizeof(SCROLLINFO);
	scrollInfo.fMask		= SIF_POS;
	scrollInfo.nPos			= m_offsetY;
	SetScrollInfo(m_hWnd, SB_VERT, &scrollInfo, TRUE);
}

void CSpuRegView::OnDeviceResetting()
{
	m_font->OnLostDevice();
}

void CSpuRegView::OnDeviceReset()
{
	m_font->OnResetDevice();
}

void CSpuRegView::SetSpu(Iop::CSpuBase* spu)
{
	m_spu = spu;
}

void CSpuRegView::Refresh()
{
	if(m_device.IsEmpty()) return;
	if(!TestDevice()) return;

	D3DCOLOR backgroundColor = ConvertSysColor(GetSysColor(COLOR_BTNFACE));
	D3DCOLOR textColor = ConvertSysColor(GetSysColor(COLOR_WINDOWTEXT));

	m_device->Clear(0, NULL, D3DCLEAR_TARGET, backgroundColor, 1.0f, 0);
	m_device->BeginScene();

	CLineDrawer drawer(m_font, textColor, -m_offsetX, -m_offsetY);

	{
		std::tstring text = m_title + _T("  VLEFT  VRIGH  PITCH  ADDRE    ADSRL  ADSRR  ADSRVOLU   REPEA  ");
		drawer.Draw(text.c_str(), text.length());
		drawer.Feed();
	}

	if(m_spu != nullptr)
	{
		TCHAR channelStatus[Iop::CSpuBase::MAX_CHANNEL + 1];

		for(unsigned int i = 0; i < Iop::CSpuBase::MAX_CHANNEL; i++)
		{
			Iop::CSpuBase::CHANNEL& channel(m_spu->GetChannel(i));
			TCHAR temp[256];
			_stprintf(temp, _T("CH%0.2i  %0.4X   %0.4X   %0.4X   %0.6X   %0.4X   %0.4X   %0.8X   %0.6X\r\n"), 
				i, 
				channel.volumeLeft,
				channel.volumeRight,
				channel.pitch,
				channel.address,
				channel.adsrLevel,
				channel.adsrRate,
				channel.adsrVolume,
				channel.repeat);
			drawer.Draw(temp);

			TCHAR status = _T('0');
			switch(channel.status)
			{
			case Iop::CSpuBase::ATTACK:
			case Iop::CSpuBase::KEY_ON:
				status = _T('A');
				break;
			case Iop::CSpuBase::DECAY:
				status = _T('D');
				break;
			case Iop::CSpuBase::SUSTAIN:
				status = _T('S');
				break;
			case Iop::CSpuBase::RELEASE:
				status = _T('R');
				break;
			}

			channelStatus[i] = status;
		}

		drawer.Feed();

		channelStatus[Iop::CSpuBase::MAX_CHANNEL] = 0;

		{
			TCHAR temp[256];
			_stprintf(temp, _T("CH_STAT: %s"), channelStatus);
			drawer.Draw(temp);
		}

		{
			TCHAR revbStat[Iop::CSpuBase::MAX_CHANNEL + 1];

			uint32 stat = m_spu->GetChannelReverb().f;
			for(unsigned int i = 0; i < Iop::CSpuBase::MAX_CHANNEL; i++)
			{
				revbStat[i] = (stat & (1 << i)) ? _T('1') : _T('0');
			}
			revbStat[Iop::CSpuBase::MAX_CHANNEL] = 0;
	
			TCHAR temp[256];
			_stprintf(temp, _T("CH_REVB: %s"), revbStat);
			drawer.Draw(temp);
		}

	}
	m_device->EndScene();
	m_device->Present(NULL, NULL, NULL, NULL);
}

CSpuRegView::CLineDrawer::CLineDrawer(const FontPtr& font, D3DCOLOR color, int posX, int posY)
: m_posX(posX)
, m_posY(posY)
, m_font(font)
, m_color(color)
{

}

void CSpuRegView::CLineDrawer::Draw(const TCHAR* text, int length)
{
	m_font->DrawText(NULL, text, length, Framework::Win32::CRect(m_posX, m_posY, 1, 1), DT_NOCLIP, m_color);
	Feed();
}

void CSpuRegView::CLineDrawer::Feed()
{
	m_posY += 12;
}
