#ifndef _XASOUNDSTREAMDECODER_H_
#define _XASOUNDSTREAMDECODER_H_

#include "WavOutputStream.h"
#include "StdStream.h"
#include "MemStream.h"

class XaSoundStreamDecoder
{
public:
										XaSoundStreamDecoder();
	virtual								~XaSoundStreamDecoder();

	void								Execute(Framework::CStream&);

private:
	enum STATE
	{
		STATE_CHECK_HEADER,
		STATE_READ_HEADER,
		STATE_CHECK_DATA_HEADER,
		STATE_READ_DATA_HEADER,
		STATE_READ_DATA,
	};

	bool								IsPcm() const;
	void								DecodePcmSamples(uint8*, unsigned int);
	void								DecodeAdpcmSamples(uint8*);
	void								MixChannels();

	STATE								m_state;
	uint32								m_samplingRate;
	uint32								m_bitDepth;
	uint32								m_channelCount;
	uint32								m_interleaveSize;
	uint32								m_dataSize;

	int32								m_leftPredictor[2];
	int32								m_rightPredictor[2];
	int16*								m_channelBuffer[2];
	uint32								m_channelBufferSize;

	uint32								m_currentAddress;
	uint32								m_currentChannel;

#ifdef _DEBUG
	Framework::CStdStream*				m_outputFile;
	Framework::CWavOutputStream*		m_wavOutput;
#endif
};

#endif
