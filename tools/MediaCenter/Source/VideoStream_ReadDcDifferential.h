#ifndef _VIDEOSTREAM_READDCDIFFERENTIAL_H_
#define _VIDEOSTREAM_READDCDIFFERENTIAL_H_

#include "VideoStream_Program.h"

namespace VideoStream
{
	class ReadDcDifferential : public Program
	{
	public:
						ReadDcDifferential();
		virtual			~ReadDcDifferential();

		void			Reset();
		void			SetChannel(unsigned int);
		void			Execute(void*, Framework::CBitStream&);

	private:
		enum PROGRAM_STATE
		{
			STATE_READSIZE,
			STATE_READDIFF,
			STATE_DONE,
		};

		PROGRAM_STATE	m_programState;
		unsigned int	m_channel;
		uint8			m_size;
	};
}

#endif
