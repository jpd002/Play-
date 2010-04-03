#include "Psp_Audio.h"
#include "Log.h"

using namespace Psp;

#define LOGNAME	("Psp_Audio")

CAudio::CAudio(uint8* ram)
: m_ram(ram)
, m_stream(NULL)
{
	memset(&m_channels, 0, sizeof(m_channels));
}

CAudio::~CAudio()
{

}

std::string CAudio::GetName() const
{
	return "sceAudio";
}

void CAudio::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

uint32 CAudio::ChReserve(uint32 channelId, uint32 param1, uint32 param2)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "ChReserve(channelId = 0x%0.8X, param1 = 0x%0.8X, param2 = 0x%0.8X);\r\n",
		channelId, param1, param2);
#endif

	if(channelId == -1)
	{
		for(unsigned int i = 0; i < MAX_CHANNEL; i++)
		{
			const CHANNEL& channel(m_channels[i]);
			if(!channel.reserved)
			{
				channelId = i;
				break;
			}
		}
	}

	if(channelId == -1)
	{
		return -1;
	}

	if(channelId >= MAX_CHANNEL)
	{
		return -1;
	}

	CHANNEL& channel(m_channels[channelId]);
	channel.reserved = true;

	return channelId;
}

uint32 CAudio::SetChannelDataLen(uint32 channelId, uint32 length)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetChannelDataLen(channelId = %d, length = 0x%0.8X);\r\n",
		channelId, length);
#endif

	if(channelId >= MAX_CHANNEL)
	{
		return -1;
	}

	CHANNEL& channel(m_channels[channelId]);
	if(!channel.reserved)
	{
		return -1;
	}

	channel.dataLength = length;

	return 0;
}

uint32 CAudio::OutputPannedBlocking(uint32 channelId, uint32 volumeLeft, uint32 volumeRight, uint32 bufferPtr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "OutputPannedBlocking(channelId = %d, volumeLeft = 0x%0.4X, volumeRight = 0x%0.4X, bufferPtr = 0x%0.8X);\r\n",
		channelId, volumeLeft, volumeRight, bufferPtr);
#endif

	assert(bufferPtr != 0);
	if(bufferPtr == 0)
	{
		return -1;
	}

	if(channelId >= MAX_CHANNEL)
	{
		return -1;
	}

	CHANNEL& channel(m_channels[channelId]);
	if(!channel.reserved)
	{
		return -1;
	}

	uint8* buffer = m_ram + bufferPtr;
	m_stream->Write(buffer, sizeof(int16) * channel.dataLength * 2);
	return channel.dataLength;
}

void CAudio::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x5EC81C55:
		context.m_State.nGPR[CMIPS::V0].nV0 = ChReserve(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 0xCB2E439E:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetChannelDataLen(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0x13F592BC:
		context.m_State.nGPR[CMIPS::V0].nV0 = OutputPannedBlocking(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X\r\n", methodId);
		break;
	}
}
