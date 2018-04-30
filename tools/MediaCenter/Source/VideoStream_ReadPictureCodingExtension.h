#ifndef _VIDEOSTREAM_READPICTURECODINGEXTENSION_H_
#define _VIDEOSTREAM_READPICTURECODINGEXTENSION_H_

#include "MpegVideoState.h"
#include "VideoStream_Program.h"
#include "VideoStream_ReadStructure.h"

namespace VideoStream
{
	class ReadPictureCodingExtension : public Program
	{
	public:
		ReadPictureCodingExtension();
		virtual ~ReadPictureCodingExtension();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READSTRUCT,
			STATE_DONE,
		};

		typedef ReadStructure<PICTURE_CODING_EXTENSION> ReadPictureCodingExtensionStructure;

		ReadPictureCodingExtensionStructure m_readPictureCodingExtensionProgram;
		PROGRAM_STATE m_programState;
	};
}

#endif
