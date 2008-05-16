#include "SH_OpenAL.h"
#include "alloca_def.h"

ALCint g_attrList[] = 
{
	ALC_FREQUENCY,	44100,
	0,				0
};

CSH_OpenAL::CSH_OpenAL() :
m_context(m_device, g_attrList)
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
	const unsigned int sampleCount = 44100;
	const unsigned int sampleRate = 44100;
	int16 samples[sampleCount];
	spu.Render(samples, sampleCount, 44100);

	if(m_availableBuffers.size())
	{
		ALuint buffer = *m_availableBuffers.begin();
		m_availableBuffers.pop_front();

		alBufferData(buffer, AL_FORMAT_MONO16, samples, sampleCount * sizeof(int16), sampleRate);
		alSourceQueueBuffers(m_source, 1, &buffer);
		m_source.Play();
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
