#include <stdexcept>
#include <assert.h>
#include "SH_WaveOut.h"

#define SAMPLE_RATE 44100
//#define SAMPLES_PER_UPDATE	(44 * 2)

CSH_WaveOut::CSH_WaveOut()
    : m_waveOut(NULL)
{
	InitializeWaveOut();
}

CSH_WaveOut::~CSH_WaveOut()
{
	if(m_waveOut != NULL)
	{
		DestroyWaveOut();
	}
}

CSoundHandler* CSH_WaveOut::HandlerFactory()
{
	return new CSH_WaveOut();
}

void CSH_WaveOut::InitializeWaveOut()
{
	assert(m_waveOut == NULL);

	WAVEFORMATEX waveFormat;
	memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
	waveFormat.nSamplesPerSec = SAMPLE_RATE;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.cbSize = 0;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

	if(waveOutOpen(&m_waveOut, WAVE_MAPPER, &waveFormat,
	               reinterpret_cast<DWORD_PTR>(&CSH_WaveOut::WaveOutProcStub),
	               reinterpret_cast<DWORD_PTR>(this),
	               CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		throw std::runtime_error("Couldn't initialize WaveOut device.");
	}

	for(unsigned int i = 0; i < MAX_BUFFERS; i++)
	{
		WAVEHDR& buffer(m_buffer[i]);
		memset(&buffer, 0, sizeof(WAVEHDR));
		buffer.dwFlags = WHDR_DONE;
	}
}

void CSH_WaveOut::DestroyWaveOut()
{
	assert(m_waveOut != NULL);

	waveOutReset(m_waveOut);
	waveOutClose(m_waveOut);
	for(unsigned int i = 0; i < MAX_BUFFERS; i++)
	{
		WAVEHDR& buffer(m_buffer[i]);
		if(buffer.lpData != NULL)
		{
			delete[] buffer.lpData;
		}
	}

	m_waveOut = NULL;
}

void CSH_WaveOut::WaveOutProc(HWAVEOUT waveOut, UINT message, DWORD_PTR param1, DWORD_PTR param2)
{
	//switch(message)
	//{
	//case WOM_DONE:
	//	{
	//		//Recycle buffer
	//		WAVEHDR* waveHeader = reinterpret_cast<WAVEHDR*>(param1);
	//		waveHeader->dwFlags |= WHDR_DONE;
	//	}
	//	break;
	//}
}

void CSH_WaveOut::WaveOutProcStub(HWAVEOUT waveOut, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
	reinterpret_cast<CSH_WaveOut*>(instance)->WaveOutProc(waveOut, message, param1, param2);
}

WAVEHDR* CSH_WaveOut::GetFreeBuffer()
{
	for(unsigned int i = 0; i < MAX_BUFFERS; i++)
	{
		WAVEHDR* buffer(&m_buffer[i]);
		if(buffer->dwFlags & WHDR_DONE) return buffer;
	}
	return NULL;
}

void CSH_WaveOut::Reset()
{
	if(m_waveOut != NULL)
	{
		DestroyWaveOut();
	}
	InitializeWaveOut();
}

void CSH_WaveOut::RecycleBuffers()
{
}

bool CSH_WaveOut::HasFreeBuffers()
{
	return GetFreeBuffer() != NULL;
}

void CSH_WaveOut::Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate)
{
	WAVEHDR* waveHeader = GetFreeBuffer();
	if(waveHeader == NULL) return;

	if(waveHeader->dwFlags & WHDR_PREPARED)
	{
		waveOutUnprepareHeader(m_waveOut, waveHeader, sizeof(WAVEHDR));
		waveHeader->dwFlags = 0;
		delete[] waveHeader->lpData;
		waveHeader->lpData = NULL;
	}

	size_t bufferSize = sampleCount * sizeof(int16);

	waveHeader->dwBufferLength = bufferSize;
	waveHeader->lpData = new CHAR[bufferSize];
	memcpy(waveHeader->lpData, buffer, bufferSize);

	waveOutPrepareHeader(m_waveOut, waveHeader, sizeof(WAVEHDR));
	waveOutWrite(m_waveOut, waveHeader, sizeof(WAVEHDR));
}
