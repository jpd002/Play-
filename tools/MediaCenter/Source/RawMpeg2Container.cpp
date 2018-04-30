#include "RawMpeg2Container.h"

CRawMpeg2Container::CRawMpeg2Container(Framework::CStream& inputStream)
    : m_inputStream(inputStream)
{
}

CRawMpeg2Container::~CRawMpeg2Container()
{
}

void CRawMpeg2Container::RegisterVideoStreamHandler(const VideoStreamHandler& videoStreamHandler)
{
	m_videoStreamHandler = videoStreamHandler;
}

CVideoContainer::STATUS CRawMpeg2Container::Read()
{
	if(m_inputStream.IsEOF()) return STATUS_EOF;
	static const uint32 bufferSize = 1024;
	uint8 buffer[bufferSize];
	m_inputStream.Read(buffer, bufferSize);
	if(m_videoStreamHandler)
	{
		m_videoStreamHandler(buffer, bufferSize);
	}
	return STATUS_INTERRUPTED;
}
