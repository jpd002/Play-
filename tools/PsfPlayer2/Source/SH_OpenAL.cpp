#include "SH_OpenAL.h"
#include "alloca_def.h"
#include "HighResTimer.h"
#include <assert.h>

//#define LOGGING

using namespace boost;

ALCint g_attrList[] = 
{
	ALC_FREQUENCY,	44100,
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
		ALuint* bufferNames = reinterpret_cast<ALuint*>(_alloca(sizeof(ALuint) * bufferCount));
		alSourceUnqueueBuffers(m_source, bufferCount, bufferNames);
		m_availableBuffers.insert(m_availableBuffers.begin(), bufferNames, bufferNames + bufferCount);
	}
}

bool CSH_OpenAL::HasFreeBuffers()
{
	RecycleBuffers();
	return m_availableBuffers.size() != 0;
}

void CSH_OpenAL::Update(CSpu& spu)
{
	const unsigned int minBufferLength = 16;
	unsigned int bufferLength = minBufferLength;

	//Update bufferLength worth of samples
//	unsigned int sampleCount = (44100 * bufferLength * 2) / 1000;
	unsigned int sampleCount = 1470;
	unsigned int sampleRate = 44100;
	int16* samples = reinterpret_cast<int16*>(alloca(sampleCount * sizeof(int16)));
	spu.Render(samples, sampleCount, sampleRate);

	assert(m_availableBuffers.size() != 0);
	ALuint buffer = *m_availableBuffers.begin();
	m_availableBuffers.pop_front();

	alBufferData(buffer, AL_FORMAT_STEREO16, samples, sampleCount * sizeof(int16), sampleRate);
#ifdef LOGGING
	FILE* log = fopen("log.raw", "ab");
	fwrite(samples, sampleCount * sizeof(int16), 1, log);
	fclose(log);
#endif
	alSourceQueueBuffers(m_source, 1, &buffer);
	if(m_availableBuffers.size() == 0)
	{
		ALint sourceState;
		alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);
		if(sourceState != AL_PLAYING)
		{
			m_source.Play();
		}
	}
}
