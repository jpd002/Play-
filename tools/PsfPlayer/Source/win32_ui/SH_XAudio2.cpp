#include <stdexcept>
#include <assert.h>
#include "SH_XAudio2.h"

#define SAMPLE_RATE 44100

CSH_XAudio2::CSH_XAudio2()
: m_masteringVoice(nullptr)
, m_sourceVoice(nullptr)
, m_voiceCallback(nullptr)
{
	memset(&m_buffers, 0, sizeof(m_buffers));
	InitializeXAudio2();
}

CSH_XAudio2::~CSH_XAudio2()
{
	m_sourceVoice->DestroyVoice();
	m_masteringVoice->DestroyVoice();
	delete m_voiceCallback;
}

CSoundHandler* CSH_XAudio2::HandlerFactory()
{
	return new CSH_XAudio2();
}

void CSH_XAudio2::InitializeXAudio2()
{
	HRESULT result = S_OK;

	result = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(result));

	//Instantiate XAudio2's class factory to prevent DLL from being unloaded by COM
	//Otherwise, DllCanUnloadNow always returns S_OK (can be unloaded)
	result = CoGetClassObject(__uuidof(XAudio2), CLSCTX_INPROC_SERVER, NULL, __uuidof(IClassFactory), reinterpret_cast<void**>(&m_classFactory));
	assert(SUCCEEDED(result));

	result = XAudio2Create(&m_xaudio2);
	assert(SUCCEEDED(result));

	result = m_xaudio2->CreateMasteringVoice(&m_masteringVoice);
	assert(SUCCEEDED(result));

	m_voiceCallback = new VoiceCallback(this);

	{
		WAVEFORMATEX waveFormat = {};
		memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
		waveFormat.nSamplesPerSec	= SAMPLE_RATE;
		waveFormat.wBitsPerSample	= 16;
		waveFormat.nChannels		= 2;
		waveFormat.cbSize			= 0;
		waveFormat.wFormatTag		= WAVE_FORMAT_PCM;
		waveFormat.nBlockAlign		= (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
		waveFormat.nAvgBytesPerSec	= waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

		result = m_xaudio2->CreateSourceVoice(&m_sourceVoice, &waveFormat, 0, 2.0f, m_voiceCallback);
		assert(SUCCEEDED(result));
	}

	result = m_sourceVoice->Start();
	assert(SUCCEEDED(result));
}

void CSH_XAudio2::Reset()
{
	HRESULT result = m_sourceVoice->FlushSourceBuffers();
	assert(SUCCEEDED(result));
}

void CSH_XAudio2::RecycleBuffers()
{

}

CSH_XAudio2::BUFFERINFO* CSH_XAudio2::GetFreeBuffer()
{
	for(unsigned int i = 0; i < MAX_BUFFERS; i++)
	{
		auto buffer(&m_buffers[i]);
		if(!buffer->inUse) return buffer;
	}
	return nullptr;
}

void CSH_XAudio2::OnBufferEnd(void* context)
{
	auto bufferInfo = reinterpret_cast<BUFFERINFO*>(context);
	assert(bufferInfo->inUse);
	bufferInfo->inUse = false;
}

bool CSH_XAudio2::HasFreeBuffers()
{
	return GetFreeBuffer() != nullptr;
}

void CSH_XAudio2::Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate)
{
	auto bufferInfo = GetFreeBuffer();
	if(bufferInfo == nullptr) return;

	size_t bufferSize = sampleCount * sizeof(int16);
	if(bufferSize != bufferInfo->dataSize)
	{
		delete [] bufferInfo->data;
		bufferInfo->data = new uint8[bufferSize];
		bufferInfo->dataSize = bufferSize;
	}
	memcpy(bufferInfo->data, buffer, bufferSize);
	bufferInfo->inUse = true;

	XAUDIO2_BUFFER buf = {};
	buf.AudioBytes	= bufferSize;
	buf.pAudioData	= bufferInfo->data;
	buf.pContext	= bufferInfo;
	HRESULT result = m_sourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));
}
