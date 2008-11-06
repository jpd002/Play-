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
	text += "      VLEFT  VRIGH  PITCH  ADDRE    ADSRL  ADSRR  ADSRV  REPEA  \r\n";
	text += "\r\n";

	//Channel detail
	for(unsigned int i = 0; i < CSpu::MAX_CHANNEL; i++)
	{
		CSpu::CHANNEL& channel(m_spu.GetChannel(i));
		uint32 address = CSpu::CH0_BASE + (i * 0x10);
		char temp[256];
		sprintf(temp, "CH%0.2i  %0.4X   %0.4X   %0.4X   %0.6X   %0.4X   %0.4X   %0.4X   %0.6X\r\n", 
			i, 
			channel.volumeLeft,
			channel.volumeRight,
			channel.pitch,
			channel.address * 8,
			channel.adsrLevel,
			channel.adsrRate,
			channel.adsrVolume >> 16,
			channel.repeat * 8);
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
	{
		char temp[256];
		sprintf(temp, "CH_STAT: %s\r\n", channelStatus);
		text += temp;
	}

	{
		char revbStat[CSpu::MAX_CHANNEL + 1];
		char temp[256];
		uint32 stat = m_spu.GetChannelReverb();
		for(unsigned int i = 0; i < CSpu::MAX_CHANNEL; i++)
		{
			revbStat[i] = (stat & (1 << i)) ? '1' : '0';
		}
		revbStat[CSpu::MAX_CHANNEL] = 0;

		sprintf(temp, "CH_REVB: %s\r\n", revbStat);
		text += temp;
	}

	SetDisplayText(text.c_str());
}
