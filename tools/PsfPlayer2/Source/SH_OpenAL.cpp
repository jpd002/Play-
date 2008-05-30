#include "SH_OpenAL.h"
#include "alloca_def.h"

//#define LOGGING

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
	m_source.Play();
}

CSH_OpenAL::~CSH_OpenAL()
{

}

void CSH_OpenAL::Update(CSpu& spu)
{
	if(m_lastUpdateTime == 0)
	{
		m_lastUpdateTime = clock();
		return;
	}

	const unsigned int bufferLength = 50;

	clock_t currentTime = clock();
	if((currentTime - m_lastUpdateTime) < (CLOCKS_PER_SEC * bufferLength) / 1000)
	{
		return;
	}
	m_lastUpdateTime = currentTime;

	//Update bufferLength worth of samples

	const unsigned int sampleCount = (44100 * bufferLength * 2) / 1000;
	const unsigned int sampleRate = 44100;
	int16 samples[sampleCount];
	spu.Render(samples, sampleCount, sampleRate);

	if(m_availableBuffers.size())
	{
		ALuint buffer = *m_availableBuffers.begin();
		m_availableBuffers.pop_front();

		alBufferData(buffer, AL_FORMAT_STEREO16, samples, sampleCount * sizeof(int16), sampleRate);
#ifdef LOGGING
		FILE* log = fopen("log.raw", "ab");
		fwrite(samples, sampleCount * sizeof(int16), 1, log);
		fclose(log);
#endif
		alSourceQueueBuffers(m_source, 1, &buffer);
		if(m_mustSync)
		{
			m_mustSync = false;
		}
		else
		{
			ALint sourceState;
			alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);
			if(sourceState != AL_PLAYING)
			{
				m_source.Play();
			}
		}
	}

	//Recycle buffers
	{
		unsigned int bufferCount = m_source.GetBuffersProcessed();
		if(bufferCount != 0)
		{
			ALuint* bufferNames = reinterpret_cast<ALuint*>(_alloca(sizeof(ALuint) * bufferCount));
			alSourceUnqueueBuffers(m_source, bufferCount, bufferNames);
			m_availableBuffers.insert(m_availableBuffers.begin(), bufferNames, bufferNames + bufferCount);
		}
	}
}
