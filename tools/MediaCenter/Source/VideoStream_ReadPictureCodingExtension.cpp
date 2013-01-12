#include "VideoStream_ReadPictureCodingExtension.h"

using namespace VideoStream;

ReadPictureCodingExtension::ReadPictureCodingExtension()
{
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	4,	&PICTURE_CODING_EXTENSION::fcode00));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	4,	&PICTURE_CODING_EXTENSION::fcode01));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	4,	&PICTURE_CODING_EXTENSION::fcode10));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	4,	&PICTURE_CODING_EXTENSION::fcode11));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	2,	&PICTURE_CODING_EXTENSION::intraDcPrecision));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	2,	&PICTURE_CODING_EXTENSION::pictureStructure));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::topFieldFirst));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::framePredFrameDct));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::concealmentMotionVectors));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::quantizerScaleType));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::intraVlcFormat));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::alternateScan));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::repeatFirstField));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::chroma420Type));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::progressiveFrame));
	m_readPictureCodingExtensionProgram.InsertCommand(
		ReadPictureCodingExtensionStructure::COMMAND(ReadPictureCodingExtensionStructure::COMMAND_TYPE_READ8,	1,	&PICTURE_CODING_EXTENSION::compositeDisplayFlag));

  //if (composite_display_flag)
  //{
  //  v_axis            = Get_Bits(1);
  //  field_sequence    = Get_Bits(3);
  //  sub_carrier       = Get_Bits(1);
  //  burst_amplitude   = Get_Bits(7);
  //  sub_carrier_phase = Get_Bits(8);
  //}
}

ReadPictureCodingExtension::~ReadPictureCodingExtension()
{

}

void ReadPictureCodingExtension::Reset()
{
	m_programState = STATE_INIT;
}

void ReadPictureCodingExtension::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	PICTURE_CODING_EXTENSION& pictureCodingExtension(state->pictureCodingExtension);

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
		m_readPictureCodingExtensionProgram.Reset();
		m_programState = STATE_READSTRUCT;
		continue;

Label_ReadStruct:
		m_readPictureCodingExtensionProgram.Execute(&pictureCodingExtension, stream);
		assert(pictureCodingExtension.compositeDisplayFlag == 0);
		assert(pictureCodingExtension.pictureStructure == 3);
		m_programState = STATE_DONE;
		continue;

Label_Done:
		return;
	}
}
