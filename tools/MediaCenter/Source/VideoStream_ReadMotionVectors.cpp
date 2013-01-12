#include <assert.h>
#include "VideoStream_ReadMotionVectors.h"
#include "MpegVideoState.h"
#include "MPEG2/MotionCodeTable.h"

using namespace VideoStream;

ReadMotionVectors::ReadMotionVectors()
{

}

ReadMotionVectors::~ReadMotionVectors()
{

}

void ReadMotionVectors::SetRSizes(uint8 hsize, uint8 vsize)
{
	m_motionVectorReader.SetRSizes(hsize, vsize);
}

void ReadMotionVectors::Reset()
{
	m_programState = STATE_INIT;
}

void ReadMotionVectors::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:							goto Label_Init;
		case STATE_SINGLE_READVECTOR:				goto Label_Single_ReadVector;
		case STATE_DOUBLE_FIRST_READFIELDSELECT:	goto Label_Double_First_ReadFieldSelect;
		case STATE_DOUBLE_FIRST_READVECTOR:			goto Label_Double_First_ReadVector;
		case STATE_DOUBLE_SECOND_READFIELDSELECT:	goto Label_Double_Second_ReadFieldSelect;
		case STATE_DOUBLE_SECOND_READVECTOR:		goto Label_Double_Second_ReadVector;
		case STATE_DONE:							goto Label_Done;
		default:									assert(0);
		}

Label_Init:
		if(decoderState.motionVectorCount == 1)
		{
			m_motionVectorReader.Reset();
			m_programState = STATE_SINGLE_READVECTOR;
		}
		else
		{
			m_programState = STATE_DOUBLE_FIRST_READFIELDSELECT;
		}
		continue;

Label_Single_ReadVector:
		m_motionVectorReader.Execute(context, stream);
		m_programState = STATE_DONE;
		continue;

Label_Double_First_ReadFieldSelect:
		{
			uint8 fieldSelect = static_cast<uint8>(stream.GetBits_MSBF(1));
			m_motionVectorReader.Reset();
			m_programState = STATE_DOUBLE_FIRST_READVECTOR;
		}
		continue;

Label_Double_First_ReadVector:
		m_motionVectorReader.Execute(context, stream);
		m_programState = STATE_DOUBLE_SECOND_READFIELDSELECT;
		continue;

Label_Double_Second_ReadFieldSelect:
		{
			uint8 fieldSelect = static_cast<uint8>(stream.GetBits_MSBF(1));
			m_motionVectorReader.Reset();
			m_programState = STATE_DOUBLE_SECOND_READVECTOR;
		}
		continue;

Label_Double_Second_ReadVector:
		m_motionVectorReader.Execute(context, stream);
		m_programState = STATE_DONE;
		continue;

Label_Done:
		return;
	}
}
