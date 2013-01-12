#include <assert.h>
#include "VideoStream_ReadSequenceHeader.h"

using namespace VideoStream;

ReadSequenceHeader::ReadSequenceHeader()
{

}

ReadSequenceHeader::~ReadSequenceHeader()
{

}

void ReadSequenceHeader::Reset()
{
	m_programState = STATE_INIT;
}

void ReadSequenceHeader::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READSTRUCT:				goto Label_ReadStruct;
		case STATE_CHECKREADINTRAMATRIX:	goto Label_CheckReadIntraMatrix;
		case STATE_READINTRAMATRIX:			goto Label_ReadIntraMatrix;
		case STATE_CHECKREADNONINTRAMATRIX:	goto Label_CheckReadNonIntraMatrix;
		case STATE_READNONINTRAMATRIX:		goto Label_ReadNonIntraMatrix;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		m_structureReader.Reset();
		m_programState = STATE_READSTRUCT;
		continue;

Label_ReadStruct:
		m_structureReader.Execute(state, stream);
		m_programState = STATE_CHECKREADINTRAMATRIX;
		sequenceHeader.macroblockWidth = (sequenceHeader.horizontalSize + 15) / 16;
		//Need to check interlaced mode, etc.
		sequenceHeader.macroblockHeight = (sequenceHeader.verticalSize + 15) / 16;
		sequenceHeader.macroblockMaxAddress = sequenceHeader.macroblockWidth * sequenceHeader.macroblockHeight;
		continue;

Label_CheckReadIntraMatrix:
		sequenceHeader.loadIntraQuantiserMatrix = static_cast<uint8>(stream.GetBits_MSBF(1));
		if(sequenceHeader.loadIntraQuantiserMatrix)
		{
			m_quantizerMatrixReader.Reset();
			m_quantizerMatrixReader.SetTable(sequenceHeader.intraQuantiserMatrix);
			m_programState = STATE_READINTRAMATRIX;
		}
		else
		{
			m_programState = STATE_CHECKREADNONINTRAMATRIX;
		}
		continue;

Label_ReadIntraMatrix:
		m_quantizerMatrixReader.Execute(state, stream);
		m_programState = STATE_CHECKREADNONINTRAMATRIX;
		continue;

Label_CheckReadNonIntraMatrix:
		sequenceHeader.loadNonIntraQuantiserMatrix = static_cast<uint8>(stream.GetBits_MSBF(1));
		if(sequenceHeader.loadNonIntraQuantiserMatrix)
		{
			m_quantizerMatrixReader.Reset();
			m_quantizerMatrixReader.SetTable(sequenceHeader.nonIntraQuantiserMatrix);
			m_programState = STATE_READNONINTRAMATRIX;
		}
		else
		{
			m_programState = STATE_DONE;
		}
		continue;

Label_ReadNonIntraMatrix:
		m_quantizerMatrixReader.Execute(state, stream);
		m_programState = STATE_DONE;
		continue;

Label_Done:
		return;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

ReadSequenceHeader::ReadSequenceHeaderStruct::ReadSequenceHeaderStruct()
{
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ16,		12,		&SEQUENCE_HEADER::horizontalSize));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ16,		12,		&SEQUENCE_HEADER::verticalSize));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ8,		4,		&SEQUENCE_HEADER::aspectRatio));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ8,		4,		&SEQUENCE_HEADER::frameRate));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ32,		18,		&SEQUENCE_HEADER::bitRate));
	m_commands.push_back(COMMAND(COMMAND_TYPE_CHECKMARKER,	1,		1));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ16,		10,		&SEQUENCE_HEADER::vbvBufferSize));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ8,		1,		&SEQUENCE_HEADER::constrainedParameterFlag));
}

ReadSequenceHeader::ReadSequenceHeaderStruct::~ReadSequenceHeaderStruct()
{

}

//void ReadSequenceHeader::StructureReader::Reset()
//{
//	m_currentCommandIndex = 0;
//}
//
//void ReadSequenceHeader::StructureReader::Execute(MPEG_VIDEO_STATE* state, Framework::CBitStream& stream)
//{
//	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
//	while(1)
//	{
//		if(m_currentCommandIndex == m_commands.size()) return;
//		const COMMAND& command(m_commands[m_currentCommandIndex]);
//		switch(command.type)
//		{
//		case COMMAND_TYPE_READ8:
//			{
//				assert(command.size <= 8);
//				SequenceHeaderOffset8 offset = boost::any_cast<SequenceHeaderOffset8>(command.payload);
//				sequenceHeader.*(offset) = static_cast<uint8>(stream.GetBits_MSBF(command.size));
//			}
//			break;
//		case COMMAND_TYPE_READ16:
//			{
//				assert(command.size <= 16);
//				SequenceHeaderOffset16 offset = boost::any_cast<SequenceHeaderOffset16>(command.payload);
//				sequenceHeader.*(offset) = static_cast<uint16>(stream.GetBits_MSBF(command.size));
//			}
//			break;
//		case COMMAND_TYPE_READ32:
//			{
//				assert(command.size <= 32);
//				SequenceHeaderOffset32 offset = boost::any_cast<SequenceHeaderOffset32>(command.payload);
//				sequenceHeader.*(offset) = stream.GetBits_MSBF(command.size);
//			}
//			break;
//		case COMMAND_TYPE_CHECKMARKER:
//			{
//				uint32 marker = stream.GetBits_MSBF(command.size);
//				uint32 reference = boost::any_cast<int>(command.payload);
//				assert(marker == reference);
//			}
//			break;
//		default:
//			assert(0);
//			break;
//		}
//		m_currentCommandIndex++;
//	}
//}
//
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

ReadSequenceHeader::QuantizerMatrixReader::QuantizerMatrixReader()
: m_table(NULL)
, m_currentIndex(0)
{

}

ReadSequenceHeader::QuantizerMatrixReader::~QuantizerMatrixReader()
{

}

void ReadSequenceHeader::QuantizerMatrixReader::SetTable(uint8* table)
{
	m_table = table;
}

void ReadSequenceHeader::QuantizerMatrixReader::Reset()
{
	m_currentIndex = 0;
}

void ReadSequenceHeader::QuantizerMatrixReader::Execute(void* state, Framework::CBitStream& stream)
{
	while(1)
	{
		if(m_currentIndex == 0x40) return;
		m_table[m_currentIndex] = static_cast<uint8>(stream.GetBits_MSBF(8));
		m_currentIndex++;
	}
}
