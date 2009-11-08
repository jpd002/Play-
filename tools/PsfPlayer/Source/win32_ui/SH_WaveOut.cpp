#include <stdexcept>
#include <assert.h>
#include "SH_WaveOut.h"

#define SAMPLE_RATE 44100
//#define SAMPLES_PER_UPDATE	(44 * 2)

using namespace std;

CSH_WaveOut::CSH_WaveOut() :
m_waveOut(NULL)
{
	WAVEFORMATEX waveFormat;
	memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
	waveFormat.nSamplesPerSec   = SAMPLE_RATE;
	waveFormat.wBitsPerSample   = 16;
	waveFormat.nChannels        = 2;
	waveFormat.cbSize           = 0;
	waveFormat.wFormatTag       = WAVE_FORMAT_PCM;
	waveFormat.nBlockAlign      = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec  = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

	if(waveOutOpen(&m_waveOut, WAVE_MAPPER, &waveFormat, 
		reinterpret_cast<DWORD_PTR>(&CSH_WaveOut::WaveOutProcStub), 
		reinterpret_cast<DWORD_PTR>(this), 
		CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		throw runtime_error("Couldn't initialize WaveOut device.");
	}

//	m_bufferMemory = new int16[MAX_BUFFERS * SAMPLES_PER_UPDATE];
//	memset(m_bufferMemory, 0, sizeof(int16) * MAX_BUFFERS * SAMPLES_PER_UPDATE);

	for(unsigned int i = 0; i < MAX_BUFFERS; i++)
	{
		WAVEHDR& buffer(m_buffer[i]);
		memset(&buffer, 0, sizeof(WAVEHDR));
//		buffer.dwBufferLength	= sizeof(int16) * SAMPLES_PER_UPDATE;
//		buffer.lpData			= reinterpret_cast<LPSTR>(m_bufferMemory + (i * SAMPLES_PER_UPDATE));
		buffer.dwFlags			= WHDR_DONE;
	}
}

CSH_WaveOut::~CSH_WaveOut()
{
	waveOutReset(m_waveOut);
	waveOutClose(m_waveOut);
    for(unsigned int i = 0; i < MAX_BUFFERS; i++)
    {
		WAVEHDR& buffer(m_buffer[i]);
        if(buffer.lpData != NULL)
        {
            delete [] buffer.lpData;
        }
    }
//	delete [] m_bufferMemory;
}

CSoundHandler* CSH_WaveOut::HandlerFactory()
{
	return new CSH_WaveOut();
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
	waveOutReset(m_waveOut);
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
        delete [] waveHeader->lpData;
        waveHeader->lpData = NULL;
	}

	//assert((sampleCount * 2) == SAMPLES_PER_UPDATE);

//    size_t bufferSize = sampleCount * sizeof(int16) * 2;
    size_t bufferSize = sampleCount * sizeof(int16);
//	int16* samples = reinterpret_cast<int16*>(waveHeader->lpData);
//	memcpy(samples, buffer, bufferSize);

    waveHeader->dwBufferLength  = bufferSize;
    waveHeader->lpData          = new CHAR[bufferSize];
//    waveHeader->lpData          = reinterpret_cast<LPSTR>(buffer);
    memcpy(waveHeader->lpData, buffer, bufferSize);

	waveOutPrepareHeader(m_waveOut, waveHeader, sizeof(WAVEHDR));
	waveOutWrite(m_waveOut, waveHeader, sizeof(WAVEHDR));
}
