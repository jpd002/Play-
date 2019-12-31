#pragma once

#ifdef _WIN32
#include "../../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.h"
#else
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
#endif

extern bool g_audioEnabled;


#ifdef _WIN32
class CSH_LibreAudio : public CSH_WaveOut
#else
class CSH_LibreAudio : public CSH_OpenAL
#endif
{
public:
	CSH_LibreAudio() = default;

	static CSoundHandler* HandlerFactory()
	{
		return new CSH_LibreAudio();
	}

	void Write(int16* buffer, unsigned int sampleCount, unsigned int sampleRate) override
	{
		if(g_audioEnabled)
		{	
#ifdef _WIN32
			CSH_WaveOut::Write(buffer, sampleCount, sampleRate);
#else
			CSH_OpenAL::Write(buffer, sampleCount, sampleRate);
#endif
		}
	};
};
