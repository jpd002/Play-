#include "SpuRegView.h"
#include "win32/Rect.h"

CSpuRegView::CSpuRegView(HWND parentWnd) :
CDirectXControl(parentWnd),
m_spu(NULL),
m_font(NULL)
{
	Initialize();

	SetTimer(m_hWnd, NULL, 16, NULL);
}

CSpuRegView::~CSpuRegView()
{
	if(m_font)
	{
		m_font->Release();
		m_font = NULL;
	}
}

long CSpuRegView::OnTimer(WPARAM param)
{
	if(IsWindowVisible(m_hWnd))
	{
		Refresh();
	}
	return TRUE;
}

void CSpuRegView::RecreateDevice()
{
	CDirectXControl::RecreateDevice();

	if(m_font)
	{
		m_font->Release();
		m_font = NULL;
	}

	D3DXCreateFont(m_device, -11, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"), &m_font);
}

void CSpuRegView::SetSpu(Iop::CSpuBase* spu)
{
	m_spu = spu;
}

void CSpuRegView::Refresh()
{
	if(m_device == NULL) return;

	m_device->Clear(0, NULL, D3DCLEAR_TARGET, m_backgroundColor, 1.0f, 0);
	m_device->BeginScene();
	if(m_spu != NULL)
	{
		CLineDrawer drawer(m_font, m_textColor);

		{
			std::tstring text = _T("      VLEFT  VRIGH  PITCH  ADDRE    ADSRL  ADSRR  ADSRV  REPEA  ");
			drawer.Draw(text.c_str(), text.length());
			drawer.Feed();
		}

		for(unsigned int i = 0; i < Iop::CSpuBase::MAX_CHANNEL; i++)
		{
			Iop::CSpuBase::CHANNEL& channel(m_spu->GetChannel(i));
			TCHAR temp[256];
			_stprintf(temp, _T("CH%0.2i  %0.4X   %0.4X   %0.4X   %0.6X   %0.4X   %0.4X   %0.4X   %0.6X\r\n"), 
				i, 
				channel.volumeLeft,
				channel.volumeRight,
				channel.pitch,
				channel.address,
				channel.adsrLevel,
				channel.adsrRate,
				channel.adsrVolume >> 16,
				channel.repeat);
			drawer.Draw(temp);
		}
	}
	m_device->EndScene();
	m_device->Present(NULL, NULL, NULL, NULL);
}

CSpuRegView::CLineDrawer::CLineDrawer(LPD3DXFONT font, D3DCOLOR color)
: m_posY(0)
, m_font(font)
, m_color(color)
{

}

void CSpuRegView::CLineDrawer::Draw(const TCHAR* text, int length)
{
	m_font->DrawText(NULL, text, length, Framework::Win32::CRect(0, m_posY, 1, 1), DT_NOCLIP, m_color);
	Feed();
}

void CSpuRegView::CLineDrawer::Feed()
{
	m_posY += 12;
}

//void CSpuRegView::Render()
//{
//	string text;
//	char channelStatus[CSpuBase::MAX_CHANNEL + 1];
//
//	//Header
//	text += "      VLEFT  VRIGH  PITCH  ADDRE    ADSRL  ADSRR  ADSRV  REPEA  \r\n";
//	text += "\r\n";
//
//	//Channel detail
//	for(unsigned int i = 0; i < CSpuBase::MAX_CHANNEL; i++)
//	{
//		CSpuBase::CHANNEL& channel(m_spu.GetChannel(i));
//		char temp[256];
//		sprintf(temp, "CH%0.2i  %0.4X   %0.4X   %0.4X   %0.6X   %0.4X   %0.4X   %0.4X   %0.6X\r\n", 
//			i, 
//			channel.volumeLeft,
//			channel.volumeRight,
//			channel.pitch,
//			channel.address,
//			channel.adsrLevel,
//			channel.adsrRate,
//			channel.adsrVolume >> 16,
//			channel.repeat);
//		text += temp;
//
//		char status = '0';
//		switch(channel.status)
//		{
//		case CSpuBase::ATTACK:
//		case CSpuBase::KEY_ON:
//			status = 'A';
//			break;
//		case CSpuBase::DECAY:
//			status = 'D';
//			break;
//		case CSpuBase::SUSTAIN:
//			status = 'S';
//			break;
//		case CSpuBase::RELEASE:
//			status = 'R';
//			break;
//		}
//
//		channelStatus[i] = status;
//	}
//
//	channelStatus[CSpuBase::MAX_CHANNEL] = 0;
//
//	text += "\r\n";
//	{
//		char temp[256];
//		sprintf(temp, "CH_STAT: %s\r\n", channelStatus);
//		text += temp;
//	}
//
//	{
//		char revbStat[CSpuBase::MAX_CHANNEL + 1];
//		char temp[256];
//		uint32 stat = m_spu.GetChannelReverb().f;
//		for(unsigned int i = 0; i < CSpuBase::MAX_CHANNEL; i++)
//		{
//			revbStat[i] = (stat & (1 << i)) ? '1' : '0';
//		}
//		revbStat[CSpuBase::MAX_CHANNEL] = 0;
//
//		sprintf(temp, "CH_REVB: %s\r\n", revbStat);
//		text += temp;
//	}
//
//	SetDisplayText(text.c_str());
//}
