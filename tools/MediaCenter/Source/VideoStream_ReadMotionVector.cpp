#include <assert.h>
#include "VideoStream_ReadMotionVector.h"
#include "MpegVideoState.h"
#include "MPEG2/MotionCodeTable.h"

using namespace VideoStream;

ReadMotionVector::ReadMotionVector()
{
	m_hrSize = 0;
	m_vrSize = 0;
}

ReadMotionVector::~ReadMotionVector()
{

}

void ReadMotionVector::SetRSizes(uint8 hsize, uint8 vsize)
{
	m_hrSize = hsize;
	m_vrSize = vsize;
}

void ReadMotionVector::Reset()
{
	m_programState = STATE_INIT;
}

void ReadMotionVector::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READHMOTIONCODE:			goto Label_ReadHMotionCode;
		case STATE_READHMOTIONRESIDUAL:		goto Label_ReadHMotionResidual;
		case STATE_READVMOTIONCODE:			goto Label_ReadVMotionCode;
		case STATE_READVMOTIONRESIDUAL:		goto Label_ReadVMotionResidual;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		m_programState = STATE_READHMOTIONCODE;
		continue;

Label_ReadHMotionCode:
		{
			decoderState.motionCode[0] = static_cast<int16>(MPEG2::CMotionCodeTable::GetInstance()->GetSymbol(&stream));
			decoderState.motionResidual[0] = 0;
			if(decoderState.motionCode[0] != 0 && m_hrSize != 0)
			{
				m_programState = STATE_READHMOTIONRESIDUAL;
			}
			else
			{
				m_programState = STATE_READVMOTIONCODE;
			}
		}
		continue;

Label_ReadHMotionResidual:
		{
			decoderState.motionResidual[0] = static_cast<uint16>(stream.GetBits_MSBF(m_hrSize));
			m_programState = STATE_READVMOTIONCODE;
		}
		continue;

Label_ReadVMotionCode:
		{
			decoderState.motionCode[1] = static_cast<int16>(MPEG2::CMotionCodeTable::GetInstance()->GetSymbol(&stream));
			decoderState.motionResidual[1] = 0;
			if(decoderState.motionCode[1] != 0 && m_vrSize != 0)
			{
				m_programState = STATE_READVMOTIONRESIDUAL;
			}
			else
			{
				m_programState = STATE_DONE;
			}
		}
		continue;

Label_ReadVMotionResidual:
		{
			decoderState.motionResidual[1] = static_cast<uint16>(stream.GetBits_MSBF(m_vrSize));
			m_programState = STATE_DONE;
		}
		continue;

Label_Done:
		return;
	}
}
