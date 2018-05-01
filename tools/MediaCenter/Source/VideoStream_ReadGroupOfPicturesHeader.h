#ifndef _VIDEOSTREAM_READGROUPOFPICTURESHEADER_H_
#define _VIDEOSTREAM_READGROUPOFPICTURESHEADER_H_

#include "MpegVideoState.h"
#include "VideoStream_ReadStructure.h"

namespace VideoStream
{
	class ReadGroupOfPicturesHeader : public ReadStructure<GOP_HEADER>
	{
	public:
		ReadGroupOfPicturesHeader();
		virtual ~ReadGroupOfPicturesHeader();
		virtual void Execute(void*, Framework::CBitStream&);

	private:
	};
};

#endif
