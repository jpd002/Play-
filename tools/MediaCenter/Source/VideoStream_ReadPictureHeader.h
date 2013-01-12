#ifndef _VIDEOSTREAM_READPICTUREHEADER_H_
#define _VIDEOSTREAM_READPICTUREHEADER_H_

#include "MpegVideoState.h"
#include "VideoStream_Program.h"
#include "VideoStream_ReadStructure.h"

namespace VideoStream
{
	class ReadPictureHeader : public Program
	{
	public:
										ReadPictureHeader();
		virtual							~ReadPictureHeader();

		void							Reset();
		void							Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READSTRUCT,
			STATE_READPELFORWARDVECTOR,
			STATE_READFORWARDFCODE,
			STATE_READPELBACKWARDVECTOR,
			STATE_READBACKWARDFCODE,
			STATE_DECODEEXTRABITS,
			STATE_FINISHEXTRABITS,
			STATE_DONE,
		};

		typedef ReadStructure<PICTURE_HEADER> ReadPictureHeaderStructure;

		ReadPictureHeaderStructure		m_readPictureHeaderProgram;
		PROGRAM_STATE					m_programState;
	};
}

#endif
