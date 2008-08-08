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
	char channelStatus[CSpu::MAX_CHANNEL + 1];

	//Header
	text += "      VLEFT  VRIGH  PITCH  ADDRE  ADSRL  ADSRR  ADSRV  REPEA\r\n";
	text += "\r\n";

	//Channel detail
	for(unsigned int i = 0; i < CSpu::MAX_CHANNEL; i++)
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

		char status = '0';
		switch(channel.status)
		{
		case CSpu::ATTACK:
		case CSpu::KEY_ON:
			status = 'A';
			break;
		case CSpu::DECAY:
			status = 'D';
			break;
		case CSpu::SUSTAIN:
			status = 'S';
			break;
		case CSpu::RELEASE:
			status = 'R';
			break;
		}

		channelStatus[i] = status;
	}

	channelStatus[CSpu::MAX_CHANNEL] = 0;

	text += "\r\n";
	char temp[256];
	sprintf(temp, "CH_ON: 0x%0.8X CH_STAT: %s\r\n", m_spu.GetChannelOn(), channelStatus);
	text += temp;

	SetDisplayText(text.c_str());
}
