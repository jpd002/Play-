#ifndef _WAVSTREAM_H_
#define _WAVSTREAM_H_

#include "Stream.h"

namespace Framework
{
	class CWavOutputStream : public Framework::CStream
	{
	public:
		struct WAVINFO
		{
			unsigned int channelCount;
			unsigned int sampleRate;
			unsigned int bitDepth;
		};

		CWavOutputStream(Framework::CStream&);
		virtual ~CWavOutputStream();

		void Begin(const WAVINFO&);
		void End();

		void Flush();

		virtual void Seek(int64, STREAM_SEEK_DIRECTION);
		virtual uint64 Tell();
		virtual uint64 Read(void*, uint64);
		virtual uint64 Write(const void*, uint64);
		virtual bool IsEOF();

	private:
		Framework::CStream& m_stream;
		uint64 m_chunkSizePos;
		uint64 m_dataSizePos;
		uint64 m_writeSize;
		bool m_begun;
	};
};

#endif
