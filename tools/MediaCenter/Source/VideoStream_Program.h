#ifndef _VIDEOSTREAM_PROGRAM_H_
#define _VIDEOSTREAM_PROGRAM_H_

#include "BitStream.h"

namespace VideoStream
{
	class Program
	{
	public:
		virtual					~Program() {}
		virtual void			Reset()										= 0;
		virtual void			Execute(void*, Framework::CBitStream&)		= 0;
	};
}

#endif
