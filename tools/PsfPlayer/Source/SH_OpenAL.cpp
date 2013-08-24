#include "SH_OpenAL.h"
#include "alloca_def.h"
#include <assert.h>

//#define LOGGING
#define SAMPLE_RATE 44100

ALCint g_attrList[] = 
{
	ALC_FREQUENCY,	SAMPLE_RATE,
	0,				0
};

#define CHECK_AL_ERROR() { assert(alGetError() == AL_NO_ERROR); }

CSH_OpenAL::CSH_OpenAL() :
m_context(m_device, g_attrList),
m_lastUpdateTime(0),
m_mustSync(true)
{
	m_context.MakeCurrent();
	ALuint bufferNames[MAX_BUFFERS];
	alGenBuffers(MAX_BUFFERS, bufferNames);
	CHECK_AL_ERROR();
	m_availableBuffers.insert(m_availableBuffers.begin(), bufferNames, bufferNames + MAX_BUFFERS);
}

CSH_OpenAL::~CSH_OpenAL()
{

}

CSoundHandler* CSH_OpenAL::HandlerFactory()
{
	return new CSH_OpenAL();
}

void CSH_OpenAL::Reset()
{
	m_source.Stop();
	ALint sourceState = m_source.GetState();
	assert(sourceState == AL_INITIAL || sourceState == AL_STOPPED);
	RecycleBuffers();
	assert(m_availableBuffers.size() == MAX_BUFFERS);
}

void CSH_OpenAL::RecycleBuffers()
{
	unsigned int bufferCount = m_source.GetBuffersProcessed();
	if(bufferCount != 0)
	{
		ALuint* bufferNames = reinterpret_cast<ALuint*>(alloca(sizeof(ALuint) * bufferCount));
		alSourceUnqueueBuffers(m_source, bufferCount, bufferNames);
		CHECK_AL_ERROR();
		m_availableBuffers.insert(m_availableBuffers.begin(), bufferNames, bufferNames + bufferCount);
	}
}

bool CSH_OpenAL::HasFreeBuffers()
{
	return m_availableBuffers.size() != 0;
}

void CSH_OpenAL::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	assert(m_availableBuffers.size() != 0);
	if(m_availableBuffers.size() == 0) return;
	ALuint buffer = *m_availableBuffers.begin();
	m_availableBuffers.pop_front();

	alBufferData(buffer, AL_FORMAT_STEREO16, samples, sampleCount * sizeof(int16), sampleRate);
	CHECK_AL_ERROR();
	
	alSourceQueueBuffers(m_source, 1, &buffer);
	CHECK_AL_ERROR();
	
	if(m_availableBuffers.size() == 0)
	{
		ALint sourceState = m_source.GetState();
		if(sourceState != AL_PLAYING)
		{
			m_source.Play();
			assert(m_source.GetState() == AL_PLAYING);
			RecycleBuffers();
		}
	}
}
