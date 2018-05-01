#include <assert.h>
#include "VideoStream_Decoder.h"

using namespace VideoStream;
using namespace Framework;

static const uint8 g_defaultIntraQuantizerMatrix[0x40] =
    {
        8, 16, 19, 22, 26, 27, 29, 34,
        16, 16, 22, 24, 27, 29, 34, 37,
        19, 22, 26, 27, 29, 34, 34, 38,
        22, 22, 26, 27, 29, 34, 37, 40,
        22, 26, 27, 29, 32, 35, 40, 48,
        26, 27, 29, 32, 35, 40, 48, 58,
        26, 27, 29, 34, 38, 46, 56, 69,
        27, 29, 35, 38, 46, 56, 69, 83};

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
}

void Decoder::InitializeState(MPEG_VIDEO_STATE* state)
{
	memset(state, 0, sizeof(MPEG_VIDEO_STATE));
	{
		SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
		memcpy(sequenceHeader.intraQuantiserMatrix, g_defaultIntraQuantizerMatrix, sizeof(uint8) * 0x40);
		for(unsigned int i = 0; i < 0x40; i++)
		{
			sequenceHeader.nonIntraQuantiserMatrix[i] = 16;
		}
	}

	//MPEG1 defaults
	state->pictureCodingExtension.framePredFrameDct = 1;
}

void Decoder::RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler& handler)
{
	m_readSliceProgram.RegisterOnMacroblockDecodedHandler(handler);
}

void Decoder::RegisterOnPictureDecodedHandler(const OnPictureDecodedHandler& handler)
{
	m_readSliceProgram.RegisterOnPictureDecodedHandler(handler);
}

void Decoder::Reset()
{
	m_programState = STATE_GETMARKER;
	m_subProgram = NULL;
}

void Decoder::Execute(void* context, CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	while(1)
	{
		switch(m_programState)
		{
		case STATE_GETMARKER:
			goto Label_GetMarker;
		case STATE_SKIPMARKER:
			goto Label_SkipMarker;
		case STATE_GETCOMMAND:
			goto Label_GetCommand;
		case STATE_GETEXTENSIONID:
			goto Label_GetExtensionId;
		case STATE_SKIPBITS:
			goto Label_SkipBits;
		case STATE_EXECUTESUBPROGRAM:
			goto Label_ExecuteSubProgram;
		case STATE_FINISHEXECUTE:
			goto Label_FinishExecute;
		default:
			assert(0);
		}

	Label_GetMarker:
		m_marker = stream.PeekBits_MSBF(24);
		if(m_marker == 0x01)
		{
			m_programState = STATE_SKIPMARKER;
		}
		else
		{
			m_programState = STATE_SKIPBITS;
		}
		continue;

	Label_SkipMarker:
		stream.Advance(24);
		m_programState = STATE_GETCOMMAND;
		continue;

	Label_GetCommand:
		m_commandType = static_cast<uint8>(stream.GetBits_MSBF(8));
		if(m_commandType >= 0x01 && m_commandType <= 0xAF)
		{
			m_subProgram = &m_readSliceProgram;
		}
		else if(m_commandType == 0xB5)
		{
			m_programState = STATE_GETEXTENSIONID;
			continue;
		}
		else
		{
			switch(m_commandType)
			{
			case 0x00:
				m_subProgram = &m_readPictureHeader;
				break;
			case 0xB3:
				m_subProgram = &m_readSequenceHeaderProgram;
				break;
			case 0xB8:
				m_subProgram = &m_readGroupOfPicturesHeaderProgram;
				break;
			default:
				m_subProgram = NULL;
				break;
			}
		}
		if(m_subProgram != NULL)
		{
			m_subProgram->Reset();
			m_programState = STATE_EXECUTESUBPROGRAM;
		}
		else
		{
			m_programState = STATE_GETMARKER;
		}
		continue;

	Label_GetExtensionId:
		m_extensionType = static_cast<uint8>(stream.GetBits_MSBF(4));
		switch(m_extensionType)
		{
		case 0x01:
			m_subProgram = &m_readSequenceExtensionProgram;
			break;
		case 0x08:
			m_subProgram = &m_readPictureCodingExtensionProgram;
			break;
		default:
			m_subProgram = NULL;
			break;
		}
		if(m_subProgram != NULL)
		{
			m_subProgram->Reset();
			m_programState = STATE_EXECUTESUBPROGRAM;
		}
		else
		{
			m_programState = STATE_FINISHEXECUTE;
		}
		continue;

	Label_ExecuteSubProgram:
		m_subProgram->Execute(state, stream);
		m_programState = STATE_FINISHEXECUTE;
		continue;

	Label_FinishExecute:
		stream.SeekToByteAlign();
		m_programState = STATE_GETMARKER;
		continue;

	Label_SkipBits:
		stream.Advance(8);
		m_programState = STATE_GETMARKER;
		continue;
	}
}
