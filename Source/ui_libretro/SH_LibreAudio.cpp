#include "SH_LibreAudio.h"
#include "libretro.h"

extern bool g_audioEnabled;
extern retro_audio_sample_batch_t g_set_audio_sample_batch_cb;

CSoundHandler* CSH_LibreAudio::HandlerFactory()
{
	return new CSH_LibreAudio();
}

void CSH_LibreAudio::Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate)
{
	if(g_audioEnabled)
	{
		std::vector<int16> buf(sampleCount * sizeof(int16));
		memcpy(buf.data(), buffer, sampleCount * sizeof(int16));
		m_queue.push_back(std::move(buf));
	}
}

void CSH_LibreAudio::ProcessBuffer()
{
	if(!m_queue.empty())
	{
		auto buf = std::move(m_queue.front());
		m_queue.pop_front();
		if(g_set_audio_sample_batch_cb)
			g_set_audio_sample_batch_cb(buf.data(), buf.size() / (2 * sizeof(int16)));
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
