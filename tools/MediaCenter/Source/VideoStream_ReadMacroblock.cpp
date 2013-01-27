#include <assert.h>
#include "MpegVideoState.h"
#include "VideoStream_ReadMacroblock.h"
#include "MPEG2/MacroblockAddressIncrementTable.h"
#include "MPEG2/CodedBlockPatternTable.h"

using namespace VideoStream;

ReadMacroblock::ReadMacroblock()
{

}

ReadMacroblock::~ReadMacroblock()
{

}

void ReadMacroblock::RegisterOnMacroblockDecodedHandler(const OnMacroblockDecodedHandler& handler)
{
	m_OnMacroblockDecodedHandler = handler;
}

void ReadMacroblock::Reset()
{
	m_programState = STATE_INIT;
}

void ReadMacroblock::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_HEADER& pictureHeader(state->pictureHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_ESCAPE:					goto Label_Escape;
		case STATE_SKIPESCAPE:				goto Label_SkipEscape;
		case STATE_READMBINCREMENT:			goto Label_ReadMbIncrement;
		case STATE_READMBMODES:				goto Label_ReadMbModes;
		case STATE_READDCTTYPE:				goto Label_ReadDctType;
		case STATE_CHECKMBMODES:			goto Label_CheckMbModes;
		case STATE_CHECKMBMODES_QSC:		goto Label_CheckMbModes_Qsc;
		case STATE_CHECKMBMODES_FWM_INIT:	goto Label_CheckMbModes_Fwm_Init;
		case STATE_CHECKMBMODES_FWM:		goto Label_CheckMbModes_Fwm;
		case STATE_CHECKMBMODES_BKM_INIT:	goto Label_CheckMbModes_Bkm_Init;
		case STATE_CHECKMBMODES_BKM:		goto Label_CheckMbModes_Bkm;
		case STATE_CHECKMBMODES_CBP:		goto Label_CheckMbModes_Cbp;
		case STATE_READBLOCKINIT:			goto Label_ReadBlockInit;
		case STATE_READBLOCK:				goto Label_ReadBlock;
		default:							assert(0);
		}

Label_Init:
		decoderState.mbIncrement = 0;
		m_programState = STATE_ESCAPE;
		continue;

Label_Escape:
		{
			uint32 escapeSequence = stream.PeekBits_MSBF(11);
			if(escapeSequence != 0x08)
			{
				m_programState = STATE_READMBINCREMENT;
			}
			else
			{
				m_programState = STATE_SKIPESCAPE;
			}
		}
		continue;

Label_SkipEscape:
		stream.Advance(11);
		decoderState.mbIncrement += 33;
		m_programState = STATE_ESCAPE;
		continue;

Label_ReadMbIncrement:
		decoderState.mbIncrement += MPEG2::CMacroblockAddressIncrementTable::GetInstance()->GetSymbol(&stream);
		if(decoderState.mbIncrement != 1)
		{
			decoderState.currentMbAddress += (decoderState.mbIncrement - 1);
			decoderState.mbIncrement = 1;

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

			if((pictureHeader.pictureCodingType == PICTURE_TYPE_P))
			{
				decoderState.forwardMotionVector[0] = 0;
				decoderState.forwardMotionVector[1] = 0;
			}
		}
		m_macroblockModesReader.Reset();
		m_programState = STATE_READMBMODES;
		continue;

Label_ReadMbModes:
		m_macroblockModesReader.Execute(context, stream);
		if(
			(!pictureCodingExtension.framePredFrameDct) &&
			(decoderState.macroblockType & (MACROBLOCK_MODE_BLOCK_PATTERN | MACROBLOCK_MODE_INTRA))
			)
		{
			m_programState = STATE_READDCTTYPE;
		}
		else
		{
			m_programState = STATE_CHECKMBMODES;
		}
		continue;

Label_ReadDctType:
		decoderState.dctType = static_cast<uint8>(stream.GetBits_MSBF(1));
		m_programState = STATE_CHECKMBMODES;
		continue;

Label_CheckMbModes:
		m_programState = STATE_CHECKMBMODES_QSC;
		continue;

Label_CheckMbModes_Qsc:
		if(decoderState.macroblockType & MACROBLOCK_MODE_QUANT)
		{
			decoderState.quantizerScaleCode = static_cast<uint8>(stream.GetBits_MSBF(5));
		}
		m_programState = STATE_CHECKMBMODES_FWM_INIT;
		continue;

Label_CheckMbModes_Fwm_Init:
		if(sequenceHeader.isMpeg2)
		{
			m_motionVectorsReader.Reset();
			m_motionVectorsReader.SetRSizes(pictureCodingExtension.fcode00 - 1, pictureCodingExtension.fcode01 - 1);
		}
		else
		{
			m_singleMotionVectorReader.Reset();
			m_singleMotionVectorReader.SetRSizes(pictureHeader.forwardFCode - 1, pictureHeader.forwardFCode - 1);
		}
		m_programState = STATE_CHECKMBMODES_FWM;
		continue;

Label_CheckMbModes_Fwm:
		if(decoderState.macroblockType & MACROBLOCK_MODE_MOTION_FORWARD) //or intra & concealment motion vectors
		{
			if(sequenceHeader.isMpeg2)
			{
				m_motionVectorsReader.SetMotionVector(decoderState.forwardMotionVector);
				m_motionVectorsReader.Execute(context, stream);
			}
			else
			{
				m_singleMotionVectorReader.Execute(context, stream);
				decoderState.forwardMotionVector[0] = 
					ReadMotionVector::ComputeMotionVector(decoderState.forwardMotionVector[0], decoderState.motionCode[0], decoderState.motionResidual[0], pictureHeader.forwardFCode - 1);
				decoderState.forwardMotionVector[1] = 
					ReadMotionVector::ComputeMotionVector(decoderState.forwardMotionVector[1], decoderState.motionCode[1], decoderState.motionResidual[1], pictureHeader.forwardFCode - 1);
			}
		}
		m_programState = STATE_CHECKMBMODES_BKM_INIT;
		continue;

Label_CheckMbModes_Bkm_Init:
		if(sequenceHeader.isMpeg2)
		{
			m_motionVectorsReader.Reset();
			m_motionVectorsReader.SetRSizes(pictureCodingExtension.fcode10 - 1, pictureCodingExtension.fcode11 - 1);
		}
		else
		{
			m_singleMotionVectorReader.Reset();
			m_singleMotionVectorReader.SetRSizes(pictureHeader.backwardFCode - 1, pictureHeader.backwardFCode - 1);
		}
		m_programState = STATE_CHECKMBMODES_BKM;
		continue;

Label_CheckMbModes_Bkm:
		if(decoderState.macroblockType & MACROBLOCK_MODE_MOTION_BACKWARD)
		{
			if(sequenceHeader.isMpeg2)
			{
				m_motionVectorsReader.SetMotionVector(decoderState.backwardMotionVector);
				m_motionVectorsReader.Execute(context, stream);
			}
			else
			{
				m_singleMotionVectorReader.Execute(context, stream);
			}
		}
		m_programState = STATE_CHECKMBMODES_CBP;
		continue;

Label_CheckMbModes_Cbp:
		if(decoderState.macroblockType & MACROBLOCK_MODE_BLOCK_PATTERN)
		{
			//Need to verify that
			decoderState.codedBlockPattern = static_cast<uint8>(MPEG2::CCodedBlockPatternTable::GetInstance()->GetSymbol(&stream));
		}
		else
		{
			if(
				(pictureHeader.pictureCodingType == PICTURE_TYPE_I) ||
				(decoderState.macroblockType & MACROBLOCK_MODE_INTRA)
				)
			{
				decoderState.codedBlockPattern = 0x3F;
			}
			else
			{
				decoderState.codedBlockPattern = 0;
			}
		}
		m_programState = STATE_READBLOCKINIT;
		continue;

Label_ReadBlockInit:
		m_blockReader.Reset();
		m_programState = STATE_READBLOCK;
		continue;

Label_ReadBlock:
		m_blockReader.Execute(context, stream);
		if(!(decoderState.macroblockType & MACROBLOCK_MODE_INTRA))
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
		if((pictureHeader.pictureCodingType == PICTURE_TYPE_P) && !(decoderState.macroblockType & (MACROBLOCK_MODE_INTRA | MACROBLOCK_MODE_MOTION_FORWARD)))
		{
			decoderState.forwardMotionVector[0] = 0;
			decoderState.forwardMotionVector[1] = 0;
		}
		if((decoderState.macroblockType & MACROBLOCK_MODE_INTRA) && (pictureCodingExtension.concealmentMotionVectors == 0))
		{
			decoderState.forwardMotionVector[0] = 0;
			decoderState.forwardMotionVector[1] = 0;
			decoderState.backwardMotionVector[0] = 0;
			decoderState.backwardMotionVector[1] = 0;
		}
		if(m_OnMacroblockDecodedHandler)
		{
			m_OnMacroblockDecodedHandler(state);
		}
		decoderState.currentMbAddress += decoderState.mbIncrement;
		return;
	}
}
