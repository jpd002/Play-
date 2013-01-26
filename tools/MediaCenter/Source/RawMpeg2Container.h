#pragma once

#include "Stream.h"
#include "VideoContainer.h"
#include <functional>

class CRawMpeg2Container : public CVideoContainer
{
public:
	typedef std::function<void (uint8*, uint32)> VideoStreamHandler;

							CRawMpeg2Container(Framework::CStream&);
	virtual					~CRawMpeg2Container();

	virtual STATUS			Read();

	void					RegisterVideoStreamHandler(const VideoStreamHandler&);

private:
	Framework::CStream&		m_inputStream;
	VideoStreamHandler		m_videoStreamHandler;
};
