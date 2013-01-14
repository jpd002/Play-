#pragma once

#include "MpegVideoState.h"
#include "Bitmap.h"
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <thread>

class CVideoDecoder : public boost::noncopyable
{
public:
	typedef boost::signals2::signal<void (const Framework::CBitmap&)> NewFrameEvent;

							CVideoDecoder(std::string);
	virtual					~CVideoDecoder();

	NewFrameEvent			NewFrame;

private:
							CVideoDecoder(CVideoDecoder&&) {}
	CVideoDecoder&			operator =(CVideoDecoder&&) {}

	void					DecoderThreadProc(std::string);
	void					OnMacroblockDecoded(MPEG_VIDEO_STATE*);
	static void				CopyBlock(Framework::CBitmap&, unsigned int, unsigned int, uint32*);

	std::thread				m_decoderThread;
	bool					m_threadDone;
	Framework::CBitmap		m_frame;
};
