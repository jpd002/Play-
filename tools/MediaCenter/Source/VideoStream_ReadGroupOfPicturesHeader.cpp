#include "VideoStream_ReadGroupOfPicturesHeader.h"

using namespace VideoStream;

ReadGroupOfPicturesHeader::ReadGroupOfPicturesHeader()
{
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ32,		25,		&GOP_HEADER::timeCode));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ8,		1,		&GOP_HEADER::closedGop));
	m_commands.push_back(COMMAND(COMMAND_TYPE_READ8,		1,		&GOP_HEADER::brokenLink));
	m_commands.push_back(COMMAND(COMMAND_ALIGN8,			0,		0));
}

ReadGroupOfPicturesHeader::~ReadGroupOfPicturesHeader()
{

}

void ReadGroupOfPicturesHeader::Execute(void* context, Framework::CBitStream& stream)
{
	MPEG_VIDEO_STATE* state(reinterpret_cast<MPEG_VIDEO_STATE*>(context));
	GOP_HEADER* gopHeader(&state->gopHeader);
	ReadStructure<GOP_HEADER>::Execute(gopHeader, stream);
}
