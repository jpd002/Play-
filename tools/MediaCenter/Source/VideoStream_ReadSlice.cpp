#include <assert.h>
#include "VideoStream_ReadSlice.h"
#include "MpegVideoState.h"

using namespace VideoStream;

ReadSlice::ReadSlice()
{

}

ReadSlice::~ReadSlice()
{

}

void ReadSlice::RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler& handler)
{
	m_macroblockReader.RegisterOnMacroblockDecodedHandler(handler);
}

void ReadSlice::Reset()
{
	m_programState = STATE_INIT;
}

void ReadSlice::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);
	
	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READQUANTIZERSCALECODE:	goto Label_ReadQuantizerScaleCode;
		case STATE_READEXTRASLICEINFOFLAG:	goto Label_ReadExtraSliceInfoFlag;
		case STATE_CHECKEND:				goto Label_CheckEnd;
		case STATE_READMACROBLOCK:			goto Label_ReadMacroblock;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		assert(state->sequenceHeader.verticalSize <= 2800);
		{
			int16 resetValue = 0;
			switch(pictureCodingExtension.intraDcPrecision)
			{
				case 0:
					resetValue = 128;
					break;
				case 1:
					resetValue = 256;
					break;
				case 2:
					resetValue = 512;
					break;
				default:
					resetValue = 0;
					assert(0);
					break;
			}
			decoderState.dcPredictor[0] = resetValue;
			decoderState.dcPredictor[1] = resetValue;
			decoderState.dcPredictor[2] = resetValue;			
		}
		m_programState = STATE_READQUANTIZERSCALECODE;
		continue;

Label_ReadQuantizerScaleCode:
		decoderState.quantizerScaleCode = static_cast<uint8>(stream.GetBits_MSBF(5));
		m_programState = STATE_READEXTRASLICEINFOFLAG;
		continue;

Label_ReadExtraSliceInfoFlag:
		uint8 extraSliceInfoFlag = static_cast<uint8>(stream.GetBits_MSBF(1));
		assert(extraSliceInfoFlag == 0);
		m_programState = STATE_CHECKEND;
		continue;

Label_CheckEnd:
		if(decoderState.currentMbAddress >= sequenceHeader.macroblockMaxAddress)
		{
			m_programState = STATE_DONE;
		}
		else
		{
			if(stream.PeekBits_MSBF(23) == 0)
			{
				m_programState = STATE_DONE;
			}
			else
			{
				m_macroblockReader.Reset();
				m_programState = STATE_READMACROBLOCK;
			}
		}
		continue;

Label_ReadMacroblock:
		m_macroblockReader.Execute(state, stream);
		m_programState = STATE_CHECKEND;
		continue;

Label_Done:
		stream.SeekToByteAlign();
		return;
	}
}
