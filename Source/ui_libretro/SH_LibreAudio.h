#pragma once

#ifdef _WIN32
#include "../../tools/PsfPlayer/Source/win32_ui/SH_WaveOut.h"
#else
#include "tools/PsfPlayer/Source/SH_OpenAL.h"
#endif

extern bool g_audioEnabled;

#ifdef _WIN32
#define _CSH_PARENT_ CSH_WaveOut
#else
#define _CSH_PARENT_ CSH_OpenAL
#endif

class CSH_LibreAudio : public _CSH_PARENT_
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
			_CSH_PARENT_::Write(buffer, sampleCount, sampleRate);
		}
	};
};
