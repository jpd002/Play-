#include <cassert>
#include "Log.h"
#include "Iop_SpuBase.h"

using namespace Iop;
using namespace std;

#define LOG_NAME ("iop_spubase")

CSpuBase::CSpuBase(uint8* ram, uint32 ramSize) :
m_ram(ram),
m_ramSize(ramSize)
{
    Reset();

	//Init log table for ADSR
	memset(m_adsrLogTable, 0, sizeof(m_adsrLogTable));

	uint32 value = 3;
	uint32 columnIncrement = 1;
	uint32 column = 0;

	for(unsigned int i = 32; i < 160; i++)
	{
		if(value < 0x3FFFFFFF)
		{
			value += columnIncrement;
			column++;
			if(column == 5)
			{
				column = 1;
				columnIncrement *= 2;
			}
		}
		else
		{
			value = 0x3FFFFFFF;
		}
		m_adsrLogTable[i] = value;
	}
}

CSpuBase::~CSpuBase()
{

}

void CSpuBase::Reset()
{
	m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;
	m_bufferAddr = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkStartAddr = 0;
    m_reverbWorkEndAddr = 0;
	m_baseSamplingRate = 44100;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_reverb, 0, sizeof(m_reverb));
}

void CSpuBase::SetBaseSamplingRate(uint32 samplingRate)
{
	m_baseSamplingRate = samplingRate;
}

uint32 CSpuBase::GetTransferAddress() const
{
	return m_bufferAddr;
}

void CSpuBase::SetTransferAddress(uint32 value)
{
	m_bufferAddr = value & (m_ramSize - 1);
}

uint32 CSpuBase::GetChannelOn() const
{
	return m_channelOn.f;
}

uint32 CSpuBase::GetChannelReverb() const
{
	return m_channelReverb.f;
}

CSpuBase::CHANNEL& CSpuBase::GetChannel(unsigned int channelNumber)
{
    assert(channelNumber < MAX_CHANNEL);
	return m_channel[channelNumber];
}

void CSpuBase::SendKeyOn(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			channel.status = KEY_ON;
		}
	}
}

void CSpuBase::SendKeyOff(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			if(channel.status == STOPPED) continue;
			if(channel.status == KEY_ON)
			{
				channel.status = STOPPED;
			}
			else
			{
				channel.status = RELEASE;
			}
		}
	}
}

uint32 CSpuBase::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Receiving DMA transfer to 0x%0.8X. Size = 0x%0.8X bytes.\r\n", 
		m_bufferAddr, blockSize * blockAmount);
#endif
	unsigned int blocksTransfered = 0;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		uint32 copySize = min<uint32>(m_ramSize - m_bufferAddr, blockSize);
		memcpy(m_ram + m_bufferAddr, buffer, copySize);
		m_bufferAddr += blockSize;
		m_bufferAddr &= m_ramSize - 1;
		buffer += blockSize;
		blocksTransfered++;
	}
	return blocksTransfered;
}
