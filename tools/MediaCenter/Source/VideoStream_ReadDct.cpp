#include <assert.h>
#include "MpegVideoState.h"
#include "VideoStream_ReadDct.h"
#include "MPEG2/DctCoefficientTable0.h"
#include "MPEG2/DctCoefficientTable1.h"

using namespace VideoStream;

ReadDct::ReadDct()
: m_coeffTable(NULL)
, m_block(NULL)
{

}

ReadDct::~ReadDct()
{

}

void ReadDct::Reset()
{
	m_programState = STATE_INIT;
	m_coeffTable = NULL;
	m_blockIndex = 0;
}

void ReadDct::SetBlockInfo(int16* block, unsigned int channel)
{
	m_channel = channel;
	m_block = block;
}

void ReadDct::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
	bool isMpeg2 = sequenceHeader.isMpeg2;

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READDCDIFFERENTIAL:		goto Label_ReadDcDifferential;
		case STATE_CHECKEOB:				goto Label_CheckEob;
		case STATE_READCOEFF:				goto Label_ReadCoeff;
		case STATE_SKIPEOB:					goto Label_SkipEob;
		default:							assert(0);
		}

Label_Init:
		{
			bool isIntra = (pictureHeader.pictureCodingType == PICTURE_TYPE_I || decoderState.macroblockType & MACROBLOCK_MODE_INTRA);

			m_dcDiffReader.Reset();
			if(pictureCodingExtension.intraVlcFormat && isIntra)
			{
				m_coeffTable = MPEG2::CDctCoefficientTable1::GetInstance();
			}
			else
			{
				m_coeffTable = MPEG2::CDctCoefficientTable0::GetInstance();
			}

			if(isIntra)
			{
				m_programState = STATE_READDCDIFFERENTIAL;
			}
			else
			{
				m_programState = STATE_CHECKEOB;
			}
		}
		continue;

Label_ReadDcDifferential:
		m_dcDiffReader.SetChannel(m_channel);
		m_dcDiffReader.Execute(context, stream);
		m_block[0] = static_cast<int16>(decoderState.dcPredictor[m_channel] + decoderState.dcDifferential);
		decoderState.dcPredictor[m_channel] = m_block[0];
		m_blockIndex++;
		m_programState = STATE_CHECKEOB;
		continue;

Label_CheckEob:
		if((m_blockIndex != 0) && m_coeffTable->IsEndOfBlock(&stream))
		{
			m_programState = STATE_SKIPEOB;
		}
		else
		{
			m_programState = STATE_READCOEFF;
		}
		continue;

Label_ReadCoeff:
		{
			MPEG2::RUNLEVELPAIR runLevelPair;
			if(m_blockIndex == 0)
			{
				m_coeffTable->GetRunLevelPairDc(&stream, &runLevelPair, isMpeg2);
			}
			else
			{
				m_coeffTable->GetRunLevelPair(&stream, &runLevelPair, isMpeg2);
			}
			m_blockIndex += runLevelPair.nRun;
		
			if(m_blockIndex < 0x40)
			{
				m_block[m_blockIndex] = static_cast<int16>(runLevelPair.nLevel);
#ifdef _DECODE_LOGGING
	            CLog::GetInstance().Print(LOG_NAME, "[%i]:%i ", index, runLevelPair.nLevel);
#endif
			}
			else
			{
				assert(0);
				break;
			}

			m_blockIndex++;
			m_programState = STATE_CHECKEOB;
		}
		continue;

Label_SkipEob:
		m_coeffTable->SkipEndOfBlock(&stream);
		return;
	}
}
