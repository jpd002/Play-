#include "SH_OpenAL.h"
#include "alloca_def.h"
#include <assert.h>

//#define LOGGING
#define SAMPLE_RATE 44100

using namespace std;

ALCint g_attrList[] = 
{
	ALC_FREQUENCY,	SAMPLE_RATE,
	0,				0
};

CSH_OpenAL::CSH_OpenAL() :
m_context(m_device, g_attrList),
m_lastUpdateTime(0),
m_mustSync(true)
{
	m_context.MakeCurrent();
	ALuint bufferNames[MAX_BUFFERS];
	alGenBuffers(MAX_BUFFERS, bufferNames);
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
	RecycleBuffers();
}

void CSH_OpenAL::RecycleBuffers()
{
	unsigned int bufferCount = m_source.GetBuffersProcessed();
	if(bufferCount != 0)
	{
		ALuint* bufferNames = reinterpret_cast<ALuint*>(alloca(sizeof(ALuint) * bufferCount));
		alSourceUnqueueBuffers(m_source, bufferCount, bufferNames);
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

	alSourceQueueBuffers(m_source, 1, &buffer);
	if(m_availableBuffers.size() == 0)
	{
		ALint sourceState;
		alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);
		if(sourceState != AL_PLAYING)
		{
			RecycleBuffers();
			m_source.Play();
		}
	}
}
