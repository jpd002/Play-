#include "VideoStream_ReadSequenceExtension.h"

using namespace VideoStream;

ReadSequenceExtension::ReadSequenceExtension()
{
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			8,	&SEQUENCE_EXTENSION::profileAndLevel));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			1,	&SEQUENCE_EXTENSION::progressiveSequence));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			2,	&SEQUENCE_EXTENSION::chromaFormat));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			2,	&SEQUENCE_EXTENSION::horizontalSizeExtension));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			2,	&SEQUENCE_EXTENSION::verticalSizeExtension));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ16,		12,	&SEQUENCE_EXTENSION::bitRateExtension));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_CHECKMARKER,	1,	1));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			8,	&SEQUENCE_EXTENSION::vbvBufferSizeExtension));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			1,	&SEQUENCE_EXTENSION::lowDelay));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			2,	&SEQUENCE_EXTENSION::frameRateExtensionN));
	m_readSequenceExtensionProgram.InsertCommand(
		ReadSequenceExtensionStructure::COMMAND(ReadSequenceExtensionStructure::COMMAND_TYPE_READ8,			5,	&SEQUENCE_EXTENSION::frameRateExtensionD));
}

ReadSequenceExtension::~ReadSequenceExtension()
{

}

void ReadSequenceExtension::Reset()
{
	m_programState = STATE_INIT;
}

void ReadSequenceExtension::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	SEQUENCE_HEADER& sequenceHeader(state->sequenceHeader);
	SEQUENCE_EXTENSION& sequenceExtension(state->sequenceExtension);
	BLOCK_DECODER_STATE& decoderState(state->blockDecoderState);

	while(1)
	{
		switch(m_programState)
		{
		case STATE_INIT:					goto Label_Init;
		case STATE_READSTRUCT:				goto Label_ReadStruct;
		case STATE_DONE:					goto Label_Done;
		default:							assert(0);
		}

Label_Init:
		sequenceHeader.isMpeg2 = true;
		m_readSequenceExtensionProgram.Reset();
		m_programState = STATE_READSTRUCT;
		continue;

Label_ReadStruct:
		m_readSequenceExtensionProgram.Execute(&sequenceExtension, stream);
		assert(sequenceExtension.chromaFormat == CHROMA_420);
		m_programState = STATE_DONE;
		continue;

Label_Done:
		return;
	}
}
