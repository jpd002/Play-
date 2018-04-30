#include <assert.h>
#include "MpegVideoState.h"
#include "VideoStream_ReadDcDifferential.h"
#include "MPEG2/DcSizeLuminanceTable.h"
#include "MPEG2/DcSizeChrominanceTable.h"

using namespace VideoStream;

ReadDcDifferential::ReadDcDifferential()
    : m_channel(0)
    , m_size(0)
{
}

ReadDcDifferential::~ReadDcDifferential()
{
}

void ReadDcDifferential::SetChannel(unsigned int channel)
{
	m_channel = channel;
}

void ReadDcDifferential::Reset()
{
	m_size = 0;
	m_programState = STATE_READSIZE;
}

void ReadDcDifferential::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_READSIZE:
			goto Label_ReadSize;
		case STATE_READDIFF:
			goto Label_ReadDiff;
		case STATE_DONE:
			goto Label_Done;
		default:
			assert(0);
		}

	Label_ReadSize:
		switch(m_channel)
		{
		case 0:
			m_size = static_cast<uint8>(MPEG2::CDcSizeLuminanceTable::GetInstance()->GetSymbol(&stream));
			break;
		case 1:
		case 2:
			m_size = static_cast<uint8>(MPEG2::CDcSizeChrominanceTable::GetInstance()->GetSymbol(&stream));
			break;
		}
		m_programState = STATE_READDIFF;
		continue;

	Label_ReadDiff:
		if(m_size == 0)
		{
			decoderState.dcDifferential = 0;
		}
		else
		{
			int16 nHalfRange = (1 << (m_size - 1));
			int16 result = static_cast<int16>(stream.GetBits_MSBF(m_size));

			if(result < nHalfRange)
			{
				result = (result + 1) - (2 * nHalfRange);
			}

			decoderState.dcDifferential = result;
		}
		m_programState = STATE_DONE;
		continue;

	Label_Done:
		return;
	}
}
