#ifndef _VIDEOSTREAM_READSEQUENCEEXTENSION_H_
#define _VIDEOSTREAM_READSEQUENCEEXTENSION_H_

#include "MpegVideoState.h"
#include "VideoStream_Program.h"
#include "VideoStream_ReadStructure.h"

namespace VideoStream
{
	class ReadSequenceExtension : public Program
	{
	public:
		ReadSequenceExtension();
		virtual ~ReadSequenceExtension();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READSTRUCT,
			STATE_DONE,
		};

		typedef ReadStructure<SEQUENCE_EXTENSION> ReadSequenceExtensionStructure;

		ReadSequenceExtensionStructure m_readSequenceExtensionProgram;
		PROGRAM_STATE m_programState;
	};
};

#endif
