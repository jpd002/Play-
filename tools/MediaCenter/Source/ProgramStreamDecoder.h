#ifndef _PROGRAMSTREAMDECODER_H_
#define _PROGRAMSTREAMDECODER_H_

#include <functional>
#include "BitStream.h"

class ProgramStreamDecoder
{
public:
	enum STATUS
	{
		STATUS_INTERRUPTED,
		STATUS_ERROR,
		STATUS_EOF,
	};

	struct PRIVATE_STREAM1_INFO
	{
		uint8	subStreamNumber;
		uint8	frameCount;
		uint16	firstAccessUnit;
	};

	typedef std::function<bool (Framework::CBitStream&, uint32)> VideoStreamHandler;
	typedef std::function<bool (Framework::CBitStream&, const PRIVATE_STREAM1_INFO&, uint32)> PrivateStreamHandler;

							ProgramStreamDecoder();
	virtual					~ProgramStreamDecoder();

	STATUS					Decode(Framework::CBitStream&);

	void					RegisterVideoStreamHandler(const VideoStreamHandler&);
	void					RegisterPrivateStream1Handler(const PrivateStreamHandler&);

private:
	uint32					ReadPesExtension(Framework::CBitStream&);

	void					ReadProgramStreamPackHeader(Framework::CBitStream&);
	void					ReadSystemHeader(Framework::CBitStream&);
	bool					ReadPrivateStream1(Framework::CBitStream&);
	void					ReadPrivateStream2(Framework::CBitStream&);
	bool					ReadVideoStream(Framework::CBitStream&);

	static void				ReadAndValidateMarker(Framework::CBitStream&, unsigned int, uint32);

	VideoStreamHandler		m_videoStreamHandler;
	PrivateStreamHandler	m_privateStream1Handler;
};

#endif
