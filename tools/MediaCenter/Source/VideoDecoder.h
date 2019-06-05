#pragma once

#include "MpegVideoState.h"
#include "bitmap/Bitmap.h"
#include <boost/signals2.hpp>
#include <thread>

class CVideoDecoder
{
public:
	typedef boost::signals2::signal<void(const Framework::CBitmap&)> NewFrameEvent;

	CVideoDecoder(std::string);
	virtual ~CVideoDecoder();

	CVideoDecoder(const CVideoDecoder&) = delete;
	CVideoDecoder& operator=(const CVideoDecoder&) = delete;

	NewFrameEvent NewFrame;

private:
	struct FRAME
	{
	public:
		FRAME();
		FRAME(FRAME&&);
		FRAME(unsigned int, unsigned int);
		~FRAME();

		FRAME& operator=(FRAME&&);

		bool IsEmpty() const;
		unsigned int GetWidth() const;
		unsigned int GetHeight() const;

		Framework::CBitmap y;
		Framework::CBitmap cr;
		Framework::CBitmap cb;
	};

	CVideoDecoder(CVideoDecoder&&)
	{
	}
	CVideoDecoder& operator=(CVideoDecoder&&)
	{
	}

	void DecoderThreadProc(std::string);
	void OnMacroblockDecoded(MPEG_VIDEO_STATE*);
	void OnPictureDecoded(MPEG_VIDEO_STATE*);
	static void CopyBlock8x8(Framework::CBitmap&, unsigned int, unsigned int, const int16*);
	static void CopyBlock8x8(int16*, unsigned int, unsigned int, int16, int16, const Framework::CBitmap&);
	static void CopyMacroblock(Framework::CBitmap&, unsigned int, unsigned int, const uint32*);

	std::thread m_decoderThread;
	bool m_threadDone;

	FRAME m_previousFrame;
	FRAME m_currentFrame;

	Framework::CBitmap m_frame;
};
