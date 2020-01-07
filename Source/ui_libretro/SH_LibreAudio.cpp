#include "SH_LibreAudio.h"
#include "libretro.h"
#include <cstring>
#include <mutex>

extern retro_audio_sample_batch_t g_set_audio_sample_batch_cb;
std::mutex m_buffer_lock;

CSoundHandler* CSH_LibreAudio::HandlerFactory()
{
	return new CSH_LibreAudio();
}

void CSH_LibreAudio::Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate)
{
	std::lock_guard<std::mutex> lock(m_buffer_lock);
	m_buffer.resize(sampleCount * sizeof(int16));
	memcpy(m_buffer.data(), buffer, sampleCount * sizeof(int16));
}

void CSH_LibreAudio::ProcessBuffer()
{
	if(!m_buffer.empty())
	{
		std::lock_guard<std::mutex> lock(m_buffer_lock);
		if(g_set_audio_sample_batch_cb)
			g_set_audio_sample_batch_cb(m_buffer.data(), m_buffer.size() / (2 * sizeof(int16)));
		m_buffer.clear();
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
