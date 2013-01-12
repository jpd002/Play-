#include "VideoStream_ReadPictureHeader.h"

using namespace VideoStream;

ReadPictureHeader::ReadPictureHeader()
{
	m_readPictureHeaderProgram.InsertCommand(
		ReadPictureHeaderStructure::COMMAND(ReadPictureHeaderStructure::COMMAND_TYPE_READ16,	10,		&PICTURE_HEADER::temporalReference));
	m_readPictureHeaderProgram.InsertCommand(
		ReadPictureHeaderStructure::COMMAND(ReadPictureHeaderStructure::COMMAND_TYPE_READ8,		3,		&PICTURE_HEADER::pictureCodingType));
	m_readPictureHeaderProgram.InsertCommand(
		ReadPictureHeaderStructure::COMMAND(ReadPictureHeaderStructure::COMMAND_TYPE_READ16,	16,		&PICTURE_HEADER::vbvDelay));
}

ReadPictureHeader::~ReadPictureHeader()
{

}

void ReadPictureHeader::Reset()
{
	m_programState = STATE_INIT;
}

void ReadPictureHeader::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READSTRUCT:				goto Label_ReadStruct;
		case STATE_DECODEEXTRABITS:			goto Label_DecodeExtraBits;
		case STATE_FINISHEXTRABITS:			goto Label_FinishExtraBits;
		case STATE_READPELFORWARDVECTOR:	goto Label_ReadPelForwardVector;
		case STATE_READFORWARDFCODE:		goto Label_ReadForwardFCode;
		case STATE_READPELBACKWARDVECTOR:	goto Label_ReadPelBackwardVector;
		case STATE_READBACKWARDFCODE:		goto Label_ReadBackwardFCode;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		m_readPictureHeaderProgram.Reset();
		m_programState = STATE_READSTRUCT;
		continue;

Label_ReadStruct:
		m_readPictureHeaderProgram.Execute(&pictureHeader, stream);
		decoderState.currentMbAddress = 0;
		if(pictureHeader.pictureCodingType == PICTURE_TYPE_I)
		{
			m_programState = STATE_DECODEEXTRABITS;
		}
		else if(pictureHeader.pictureCodingType == PICTURE_TYPE_B || pictureHeader.pictureCodingType == PICTURE_TYPE_P)
		{
			m_programState = STATE_READPELFORWARDVECTOR;
		}
		else
		{
			assert(0);
		}
		continue;

Label_ReadPelForwardVector:
		{
			pictureHeader.pelForwardVector = static_cast<uint8>(stream.GetBits_MSBF(1));
			m_programState = STATE_READFORWARDFCODE;
		}
		continue;

Label_ReadForwardFCode:
		{
			pictureHeader.forwardFCode = static_cast<uint8>(stream.GetBits_MSBF(3));
			if(pictureHeader.pictureCodingType == PICTURE_TYPE_B)
			{
				m_programState = STATE_READPELBACKWARDVECTOR;
			}
			else
			{
				m_programState = STATE_DECODEEXTRABITS;
			}
		}
		continue;

Label_ReadPelBackwardVector:
		{
			pictureHeader.pelBackwardVector = static_cast<uint8>(stream.GetBits_MSBF(1));
			m_programState = STATE_READBACKWARDFCODE;
		}
		continue;

Label_ReadBackwardFCode:
		{
			pictureHeader.backwardFCode = static_cast<uint8>(stream.GetBits_MSBF(3));
			m_programState = STATE_DECODEEXTRABITS;
		}
		continue;

Label_DecodeExtraBits:
		if(stream.PeekBits_MSBF(1) == 1)
		{
			assert(0);
		}
		m_programState = STATE_FINISHEXTRABITS;
		continue;

Label_FinishExtraBits:
		stream.GetBits_MSBF(1);
		m_programState = STATE_DONE;
		continue;

Label_Done:
		stream.SeekToByteAlign();
		pictureHeader.number++;
#ifdef _DEBUG
		printf("Processing picture %d.\r\n", pictureHeader.number - 1);
#endif
		return;
	}
}
