#include "SpuRegView.h"

using namespace std;

CSpuRegView::CSpuRegView(HWND parent, RECT* rect, CSpu& spu) :
CRegViewPage(parent, rect),
m_spu(spu)
{
	Render();
}

CSpuRegView::~CSpuRegView()
{

}

void CSpuRegView::Render()
{
	string text;

	//Header
	text += "      VLEFT  VRIGH  PITCH  ADDRE  ADSRL  ADSRR  ADSRV  REPEA\r\n";
	text += "\r\n";

	//Channel detail
	for(unsigned int i = 0; i < 24; i++)
	{
		CSpu::CHANNEL& channel(m_spu.GetChannel(i));
		uint32 address = CSpu::CH0_BASE + (i * 0x10);
		char temp[256];
		sprintf(temp, "CH%0.2i  %0.4X   %0.4X   %0.4X   %0.4X   %0.4X   %0.4X   %0.4X   %0.4X\r\n", 
			i, 
			channel.volumeLeft,
			channel.volumeRight,
			channel.pitch,
			channel.address,
			channel.adsrLevel,
			channel.adsrRate,
			channel.adsrVolume >> 16,
			channel.repeat);
		text += temp;
	}

	text += "\r\n";
	char temp[256];
	sprintf(temp, "CH_ON: 0x%0.8X VO_ON 0x%0.8X\r\n", m_spu.GetChannelOn(), m_spu.GetVoiceOn());
	text += temp;

	SetDisplayText(text.c_str());
}
