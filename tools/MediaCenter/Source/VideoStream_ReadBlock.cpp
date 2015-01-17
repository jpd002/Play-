#include <assert.h>
#include "MpegVideoState.h"
#include "VideoStream_ReadBlock.h"
#include "mpeg2/QuantiserScaleTable.h"
#include "mpeg2/InverseScanTable.h"
#include "idct/IEEE1180.h"

using namespace VideoStream;

ReadBlock::ReadBlock()
{

}

ReadBlock::~ReadBlock()
{

}

void ReadBlock::Reset()
{
	m_programState = STATE_RESETDC;
	m_currentBlockIndex = 0;
}

void ReadBlock::DequantizeBlock(void* context, int16* block)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);
	
	int16 nQuantScale = 0;
	
	if(pictureCodingExtension.quantizerScaleType == 0)
	{
		nQuantScale = static_cast<int16>(MPEG2::CQuantiserScaleTable::m_nTable0[decoderState.quantizerScaleCode]);
	}
	else
	{
		nQuantScale = static_cast<int16>(MPEG2::CQuantiserScaleTable::m_nTable1[decoderState.quantizerScaleCode]);
	}
	
	if(decoderState.macroblockType & MACROBLOCK_MODE_INTRA)
	{
		uint8* quantMatrix = sequenceHeader.intraQuantiserMatrix;
		int16 nIntraDcMult = 0;
		
		switch(pictureCodingExtension.intraDcPrecision)
		{
			case 0:
				nIntraDcMult = 8;
				break;
			case 1:
				nIntraDcMult = 4;
				break;
			case 2:
				nIntraDcMult = 2;
				break;
		}
		
		block[0] = nIntraDcMult * block[0];
		
		for(unsigned int i = 1; i < 64; i++)
		{
			int16 nSign = 0;
			
			if(block[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (block[i] > 0) ? 0x0001 : 0xFFFF;
			}
			
			block[i] = (block[i] * (int16)quantMatrix[i] * nQuantScale * 2) / 32;
			
			if(nSign != 0)
			{
				if((block[i] & 1) == 0)
				{
					block[i] = (block[i] - nSign) | 1;
				}
			}
		}
	}
	else
	{
		uint8* quantMatrix = sequenceHeader.nonIntraQuantiserMatrix;
		
		for(unsigned int i = 0; i < 64; i++)
		{
			int16 nSign = 0;
			
			if(block[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (block[i] > 0) ? 0x0001 : 0xFFFF;
			}
			
			block[i] = (((block[i] * 2) + nSign) * (int16)quantMatrix[i] * nQuantScale) / 32;
			
			if(nSign != 0)
			{
				if((block[i] & 1) == 0)
				{
					block[i] = (block[i] - nSign) | 1;
				}
			}
		}
	}
	
	//Saturate
	int16 nSum = 0;
	
	for(unsigned int i = 0; i < 64; i++)
	{
		if(block[i] > 2047)
		{
			block[i] = 2047;
		}
		if(block[i] < -2048)
		{
			block[i] = -2048;
		}
		nSum += block[i];
	}
	
	if(nSum & 1)
	{
		
	}
}

void ReadBlock::InverseScan(void* context, int16* block)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);
	int16 nTemp[0x40];
	
	memcpy(nTemp, block, sizeof(int16) * 0x40);
	unsigned int* pTable = pictureCodingExtension.alternateScan ? MPEG2::CInverseScanTable::m_nTable1 : MPEG2::CInverseScanTable::m_nTable0;
	
	for(unsigned int i = 0; i < 64; i++)
	{
		block[i] = nTemp[pTable[i]];
	}	
}

void ReadBlock::ProcessBlock(void* context, int16* block)
{
	InverseScan(context, block);
	DequantizeBlock(context, block);

	{
		int16 nTemp[0x40];
		memcpy(nTemp, block, sizeof(int16) * 0x40);
		IDCT::CIEEE1180::GetInstance()->Transform(nTemp, block);
	}
}

void ReadBlock::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);
	MACROBLOCK& macroblock(state->macroblock);
	
	BLOCKENTRY blocks[6] = 
	{
		{ macroblock.blockY[0],	0 },
		{ macroblock.blockY[1],	0 },
		{ macroblock.blockY[2],	0 },
		{ macroblock.blockY[3],	0 },
		{ macroblock.blockCr,	1 },
		{ macroblock.blockCb,	2 },
	};
	
	while(1)
	{
		switch(m_programState)
		{
		case STATE_RESETDC:					goto Label_ResetDc;
		case STATE_INITDECODE:				goto Label_InitDecode;
		case STATE_DECODE:					goto Label_Decode;
		case STATE_MOVENEXT:				goto Label_MoveNext;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_ResetDc:
		m_programState = STATE_INITDECODE;
		continue;

Label_InitDecode:
		{
			if(m_currentBlockIndex == 6)
			{
				//We're done decoding
				m_programState = STATE_DONE;
			}
			else
			{
				BLOCKENTRY& currentBlock(blocks[m_currentBlockIndex]);
				memset(currentBlock.block, 0, sizeof(int16) * 64);
				m_dctReader.Reset();
				m_dctReader.SetBlockInfo(currentBlock.block, currentBlock.channel);
				if((decoderState.codedBlockPattern & (1 << (5 - m_currentBlockIndex))))
				{
					m_programState = STATE_DECODE;
				}
				else
				{
					m_programState = STATE_MOVENEXT;
				}
			}
		}
		continue;

Label_Decode:
		m_dctReader.Execute(context, stream);
		m_programState = STATE_MOVENEXT;
		{
			BLOCKENTRY& currentBlock(blocks[m_currentBlockIndex]);
			ProcessBlock(context, currentBlock.block);
		}
		continue;

Label_MoveNext:
		m_currentBlockIndex++;
		m_programState = STATE_INITDECODE;
		continue;

Label_Done:
		return;
	}
}
