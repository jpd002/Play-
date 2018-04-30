#ifndef _VIDEOSTREAM_READMACROBLOCKMODES_H_
#define _VIDEOSTREAM_READMACROBLOCKMODES_H_

#include "VideoStream_Program.h"

namespace VideoStream
{
	class ReadMacroblockModes : public Program
	{
	public:
		ReadMacroblockModes();
		virtual ~ReadMacroblockModes();

		void Reset();
		void Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_INIT,
			STATE_READMBMODESI,
			STATE_READMBMODESP,
			STATE_READMBMODESB,
			STATE_CHECKMOTIONTYPE,
			STATE_DONE,
		};

		PROGRAM_STATE m_programState;
	};
}

#endif
