#include <assert.h>
#include "MpegVideoState.h"
#include "MPEG2/MacroblockTypeITable.h"
#include "MPEG2/MacroblockTypeBTable.h"
#include "MPEG2/MacroblockTypePTable.h"
#include "VideoStream_ReadMacroblockModes.h"

using namespace VideoStream;

ReadMacroblockModes::ReadMacroblockModes()
{

}

ReadMacroblockModes::~ReadMacroblockModes()
{

}

void ReadMacroblockModes::Reset()
{
	m_programState = STATE_INIT;
}

void ReadMacroblockModes::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READMBMODESI:			goto Label_ReadMbModesI;
		case STATE_READMBMODESB:			goto Label_ReadMbModesB;
		case STATE_READMBMODESP:			goto Label_ReadMbModesP;
		case STATE_CHECKMOTIONTYPE:			goto Label_CheckMotionType;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		if(pictureHeader.pictureCodingType == PICTURE_TYPE_I)
		{
			m_programState = STATE_READMBMODESI;
		}
		else if(pictureHeader.pictureCodingType == PICTURE_TYPE_P)
		{
			m_programState = STATE_READMBMODESP;
		}
		else if(pictureHeader.pictureCodingType == PICTURE_TYPE_B)
		{
			m_programState = STATE_READMBMODESB;
		}
		else
		{
			assert(0);
		}
		continue;

Label_ReadMbModesI:
		decoderState.macroblockType = static_cast<uint8>(MPEG2::CMacroblockTypeITable::GetInstance()->GetSymbol(&stream));
		assert((decoderState.macroblockType & ~0x11) == 0);
		decoderState.resetDc = true;
		m_programState = STATE_CHECKMOTIONTYPE;
		continue;

Label_ReadMbModesB:
		decoderState.macroblockType = static_cast<uint8>(MPEG2::CMacroblockTypeBTable::GetInstance()->GetSymbol(&stream));
		m_programState = STATE_CHECKMOTIONTYPE;
		continue;

Label_ReadMbModesP:
		decoderState.macroblockType = static_cast<uint8>(MPEG2::CMacroblockTypePTable::GetInstance()->GetSymbol(&stream));
		m_programState = STATE_CHECKMOTIONTYPE;
		continue;

Label_CheckMotionType:
		if(decoderState.macroblockType & (MACROBLOCK_MODE_MOTION_BACKWARD | MACROBLOCK_MODE_MOTION_FORWARD))
		{
			if(pictureCodingExtension.framePredFrameDct)
			{
				decoderState.motionType = MOTION_FRAME;
			}
			else
			{
				decoderState.motionType = static_cast<uint8>(stream.GetBits_MSBF(2));
			}
			if(decoderState.motionType == MOTION_FIELD /* && stwclass < 2 */)
			{
				decoderState.motionVectorCount = 2;
			}
			else
			{
				decoderState.motionVectorCount = 1;
			}
		}
		m_programState = STATE_DONE;
		continue;

Label_Done:
		return;
	}
}
