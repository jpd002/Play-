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
	if(g_audioEnabled && g_set_audio_sample_batch_cb)
	{
		g_set_audio_sample_batch_cb(buffer, sampleCount / 2);
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
