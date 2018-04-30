#include <assert.h>
#include <algorithm>
#include "XaSoundStreamDecoder.h"
#include "StdStream.h"

static const int xa_adpcm_table[5][2] =
    {
        {0, 0},
        {60, 0},
        {115, -52},
        {98, -55},
        {122, -60}};

XaSoundStreamDecoder::XaSoundStreamDecoder()
{
#ifdef _DEBUG
	m_outputFile = new Framework::CStdStream("out.wav", "wb");
	m_wavOutput = new Framework::CWavOutputStream(*m_outputFile);
#endif

	m_state = STATE_CHECK_HEADER;
	m_samplingRate = 0;
	m_bitDepth = 0;
	m_channelCount = 0;
	m_interleaveSize = 0;
	m_currentAddress = 0;
	m_currentChannel = 0;
	m_dataSize = 0;

	m_leftPredictor[0] = 0;
	m_leftPredictor[1] = 0;

	m_rightPredictor[0] = 0;
	m_rightPredictor[1] = 0;

	m_channelBuffer[0] = NULL;
	m_channelBuffer[1] = NULL;

	m_channelBufferSize = 0;
}

XaSoundStreamDecoder::~XaSoundStreamDecoder()
{
	delete[] m_channelBuffer[0];
	delete[] m_channelBuffer[1];

#ifdef _DEBUG
	delete m_wavOutput;
	delete m_outputFile;
#endif
}

bool XaSoundStreamDecoder::IsPcm() const
{
	return (m_bitDepth == 1);
}

void XaSoundStreamDecoder::DecodePcmSamples(uint8* buffer, unsigned int length)
{
	while(length != 0)
	{
		uint8* outBuffer(NULL);
		if(m_currentChannel == 0)
		{
			outBuffer = reinterpret_cast<uint8*>(m_channelBuffer[0]);
		}
		else
		{
			outBuffer = reinterpret_cast<uint8*>(m_channelBuffer[1]);
		}

		uint32 bufferIndex = (m_currentAddress % m_interleaveSize);
		uint32 toInterleave = m_interleaveSize - bufferIndex;
		uint32 toWrite = std::min<uint32>(toInterleave, length);

		assert((bufferIndex + toWrite) <= m_channelBufferSize * 2);
		memcpy(outBuffer + bufferIndex, buffer, toWrite);

		buffer += toWrite;
		length -= toWrite;
		m_currentAddress += toWrite;
		if((m_currentAddress % m_interleaveSize) == 0)
		{
			if(m_currentChannel == 1)
			{
				MixChannels();
			}
			m_currentChannel = (~m_currentChannel) & 1;
		}
	}
}

void XaSoundStreamDecoder::DecodeAdpcmSamples(uint8* samples)
{
	uint8 shift = samples[0] & 0xF;
	uint8 filter = samples[0] >> 4;

	shift = 0xC - shift;
	assert(filter < 5);

	int32 f1 = xa_adpcm_table[filter][0];
	int32 f2 = xa_adpcm_table[filter][1];

	int32* s1(NULL);
	int32* s2(NULL);
	int16* outBuffer(NULL);
	uint32 outBufferAddress = (m_currentAddress % m_interleaveSize);
	assert((outBufferAddress % 0x10) == 0);
	outBufferAddress = (outBufferAddress / 0x10) * 28;
	assert(outBufferAddress < m_channelBufferSize);

	if(m_currentChannel == 0)
	{
		s1 = &m_leftPredictor[0];
		s2 = &m_leftPredictor[1];
		outBuffer = m_channelBuffer[0];
	}
	else
	{
		s1 = &m_rightPredictor[0];
		s2 = &m_rightPredictor[1];
		outBuffer = m_channelBuffer[1];
	}
	outBuffer += outBufferAddress;

	for(unsigned int i = 0; i < 28; i++)
	{
		int32 sample = samples[2 + (i / 2)];

		if(i & 1)
		{
			sample = static_cast<int8>(sample) >> 4;
		}
		else
		{
			sample = static_cast<int8>(sample << 4) >> 4;
		}

		int32 result = (sample << shift) + ((*s1 * f1 + *s2 * f2 + 32) >> 6);
		result = std::min<int32>(result, 0x00007FFF);
		result = std::max<int32>(result, 0xFFFF8000);

		(*s2) = (*s1);
		(*s1) = result;

		(*outBuffer) = result;
		outBuffer++;
	}

	m_currentAddress += 0x10;
	if((m_currentAddress % m_interleaveSize) == 0)
	{
		if(m_currentChannel == 1)
		{
			MixChannels();
		}
		m_currentChannel = (~m_currentChannel) & 1;
	}
}

void XaSoundStreamDecoder::MixChannels()
{
	for(unsigned int i = 0; i < m_channelBufferSize; i++)
	{
#ifdef _DEBUG
		m_wavOutput->Write(&m_channelBuffer[0][i], 2);
		m_wavOutput->Write(&m_channelBuffer[1][i], 2);
#endif
	}
#ifdef _DEBUG
	m_wavOutput->Flush();
#endif
}

void XaSoundStreamDecoder::Execute(Framework::CStream& stream)
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_CHECK_HEADER:
		{
			//Check for header
			if(stream.GetRemainingLength() < 4) return;

			uint8 signature[4];
			stream.Read(signature, 4);
			if((signature[0] == 'S') && (signature[1] == 'S') && (signature[2] == 'h') && (signature[3] == 'd'))
			{
				m_state = STATE_READ_HEADER;
			}
		}
		break;
		case STATE_READ_HEADER:
		{
			//Read header
			if(stream.GetRemainingLength() < 28) return;

			uint32 headerSize = 0;
			stream.Read(&headerSize, 4);
			assert(headerSize == 24);
			stream.Read(&m_bitDepth, 4);
			stream.Read(&m_samplingRate, 4);
			stream.Read(&m_channelCount, 4);
			stream.Read(&m_interleaveSize, 4);

			if(IsPcm())
			{
				m_channelBufferSize = m_interleaveSize / 2;
			}
			else
			{
				assert(m_interleaveSize % 16 == 0);
				m_channelBufferSize = (m_interleaveSize / 16) * 28;
			}

			m_channelBuffer[0] = new int16[m_channelBufferSize];
			m_channelBuffer[1] = new int16[m_channelBufferSize];
			memset(m_channelBuffer[0], 0, sizeof(int16) * m_channelBufferSize);
			memset(m_channelBuffer[1], 0, sizeof(int16) * m_channelBufferSize);

			for(unsigned int i = 0; i < 8; i++)
			{
				stream.Read8();
			}

#ifdef _DEBUG
			Framework::CWavOutputStream::WAVINFO wavInfo;
			wavInfo.bitDepth = IsPcm() ? 16 : m_bitDepth;
			wavInfo.channelCount = m_channelCount;
			wavInfo.sampleRate = m_samplingRate;
			m_wavOutput->Begin(wavInfo);
#endif
			m_state = STATE_CHECK_DATA_HEADER;
		}
		break;
		case STATE_CHECK_DATA_HEADER:
		{
			if(stream.GetRemainingLength() < 4) return;

			uint8 signature[4];
			stream.Read(signature, 4);
			if((signature[0] == 'S') && (signature[1] == 'S') && (signature[2] == 'b') && (signature[3] == 'd'))
			{
				m_state = STATE_READ_DATA_HEADER;
			}
		}
		break;
		case STATE_READ_DATA_HEADER:
		{
			if(stream.GetRemainingLength() < 4) return;

			stream.Read(&m_dataSize, 4);
			m_state = STATE_READ_DATA;
		}
		break;

		case STATE_READ_DATA:
			if(IsPcm())
			{
				//Raw pcm?
				while(stream.GetRemainingLength() != 0)
				{
					const int bufferSize = 0x80;
					uint8 buffer[bufferSize];
					uint32 readSize = std::min<uint32>(stream.GetRemainingLength(), bufferSize);
					stream.Read(buffer, readSize);
					DecodePcmSamples(buffer, readSize);
				}
				return;
			}
			else
			{
				if(stream.GetRemainingLength() < 0x10) return;

				uint8 data[0x10];
				stream.Read(data, 0x10);
				DecodeAdpcmSamples(data);
			}
			break;
		}
	}
}
