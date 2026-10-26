#include "SH_LibreAudio.h"
#include "ext/libretro.h"
#include <cstring>
#include <mutex>

extern retro_audio_sample_batch_t g_set_audio_sample_batch_cb;
std::mutex m_buffer_lock;

CSoundHandler* CSH_LibreAudio::HandlerFactory()
{
	return new CSH_LibreAudio();
}

//The emulator writes on its own thread, and the frontend collects once a frame:
//queue what was written in between, so a block that arrives before the last one
//was collected is not lost. Hold no more than half a second, should the frontend
//stop collecting.
static const size_t MAX_QUEUED_SAMPLES = 44100;

void CSH_LibreAudio::Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate)
{
	std::lock_guard<std::mutex> lock(m_buffer_lock);
	if(m_buffer.size() + sampleCount > MAX_QUEUED_SAMPLES)
	{
		m_buffer.clear();
	}
	m_buffer.insert(m_buffer.end(), buffer, buffer + sampleCount);
}

void CSH_LibreAudio::ProcessBuffer()
{
	std::vector<int16> samples;
	{
		std::lock_guard<std::mutex> lock(m_buffer_lock);
		samples.swap(m_buffer);
	}
	if(!samples.empty() && g_set_audio_sample_batch_cb)
	{
		//Stereo: two samples to a frame
		g_set_audio_sample_batch_cb(samples.data(), samples.size() / 2);
	}
}

bool CSH_LibreAudio::HasFreeBuffers()
{
	return false;
}

void CSH_LibreAudio::Reset()
{
}

void CSH_LibreAudio::RecycleBuffers()
{
}
