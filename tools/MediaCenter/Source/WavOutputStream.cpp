#include <assert.h>
#include <stdexcept>
#include "WavOutputStream.h"

using namespace Framework;

CWavOutputStream::CWavOutputStream(CStream& stream)
: m_stream(stream)
, m_chunkSizePos(0)
, m_dataSizePos(0)
, m_writeSize(0)
, m_begun(false)
{

}

CWavOutputStream::~CWavOutputStream()
{
	if(m_begun)
	{
		End();
	}
}

void CWavOutputStream::Begin(const WAVINFO& wavInfo)
{
	if(m_begun)
	{
		throw std::runtime_error("Already begun output.");
	}
	m_begun = true;
	m_writeSize = 0;

	//Write the header
	const char riffSignature[4] = { 'R', 'I', 'F', 'F' };
	const char headerFormatSignature[4] = { 'W', 'A', 'V', 'E' };
	const char fmtSignature[4] = { 'f', 'm', 't', ' ' };
	const char dataSignature[4] = { 'd', 'a', 't', 'a' };

	//RIFF header
	m_stream.Write(riffSignature, 4);
	m_chunkSizePos = m_stream.Tell();
	m_stream.Write32(0);
	m_stream.Write(headerFormatSignature, 4);

	//fmt header
	m_stream.Write(fmtSignature, 4);
	//Subchunk1Size
	m_stream.Write32(16);
	//AudioFormat
	m_stream.Write16(1);
	//NumChannels
	m_stream.Write16(wavInfo.channelCount);
	//SampleRate
	m_stream.Write32(wavInfo.sampleRate);
	//ByteRate = SampleRate * NumChannels * BitsPerSample/8
	m_stream.Write32(wavInfo.sampleRate * wavInfo.channelCount * wavInfo.bitDepth / 8);
	//BlockAlign = NumChannels * BitsPerSample / 8
	m_stream.Write16(wavInfo.channelCount * wavInfo.bitDepth / 8);
	//BitsPerSample
	m_stream.Write16(wavInfo.bitDepth);

	//data header
	m_stream.Write(dataSignature, 4);
	m_dataSizePos = m_stream.Tell();
	m_stream.Write32(0);
}

void CWavOutputStream::Flush()
{
	assert(m_begun);
	assert((m_writeSize & 1) == 0);
	
	m_stream.Seek(m_chunkSizePos, STREAM_SEEK_SET);
	uint32 totalChunkSize = 4 + 24 + (8 + static_cast<uint32>(m_writeSize));
	m_stream.Write32(totalChunkSize);

	m_stream.Seek(m_dataSizePos, STREAM_SEEK_SET);
	m_stream.Write32(static_cast<uint32>(m_writeSize));

	m_stream.Seek(0, STREAM_SEEK_END);
	m_stream.Flush();
}

void CWavOutputStream::Seek(int64, STREAM_SEEK_DIRECTION)
{
	throw std::exception();
}

uint64 CWavOutputStream::Tell()
{
	throw std::exception();
}

bool CWavOutputStream::IsEOF()
{
	throw std::exception();
}

uint64 CWavOutputStream::Read(void* data, uint64 size)
{
	throw std::exception();
}

uint64 CWavOutputStream::Write(const void* data, uint64 size)
{
	assert(m_begun);
	uint64 result = m_stream.Write(data, size);
	m_writeSize += result;
	return result;
}

void CWavOutputStream::End()
{
	if(!m_begun)
	{
		throw std::runtime_error("Didn't start output yet.");
	}

	Flush();

	m_begun = false;
}
