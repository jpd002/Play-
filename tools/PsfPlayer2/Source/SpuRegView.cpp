#include "SpuRegView.h"

using namespace std;
using namespace Iop;

CSpuRegView::CSpuRegView(HWND parent, RECT* rect, CSpuBase& spu) :
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
	char channelStatus[CSpuBase::MAX_CHANNEL + 1];

	//Header
	text += "      VLEFT  VRIGH  PITCH  ADDRE    ADSRL  ADSRR  ADSRV  REPEA  \r\n";
	text += "\r\n";

	//Channel detail
	for(unsigned int i = 0; i < CSpuBase::MAX_CHANNEL; i++)
	{
		CSpuBase::CHANNEL& channel(m_spu.GetChannel(i));
		char temp[256];
		sprintf(temp, "CH%0.2i  %0.4X   %0.4X   %0.4X   %0.6X   %0.4X   %0.4X   %0.4X   %0.6X\r\n", 
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
		case CSpuBase::ATTACK:
		case CSpuBase::KEY_ON:
			status = 'A';
			break;
		case CSpuBase::DECAY:
			status = 'D';
			break;
		case CSpuBase::SUSTAIN:
			status = 'S';
			break;
		case CSpuBase::RELEASE:
			status = 'R';
			break;
		}

		channelStatus[i] = status;
	}

	channelStatus[CSpuBase::MAX_CHANNEL] = 0;

	text += "\r\n";
	{
		char temp[256];
		sprintf(temp, "CH_STAT: %s\r\n", channelStatus);
		text += temp;
	}

	{
		char revbStat[CSpuBase::MAX_CHANNEL + 1];
		char temp[256];
		uint32 stat = m_spu.GetChannelReverb().f;
		for(unsigned int i = 0; i < CSpuBase::MAX_CHANNEL; i++)
		{
			revbStat[i] = (stat & (1 << i)) ? '1' : '0';
		}
		revbStat[CSpuBase::MAX_CHANNEL] = 0;

		sprintf(temp, "CH_REVB: %s\r\n", revbStat);
		text += temp;
	}

	SetDisplayText(text.c_str());
}
