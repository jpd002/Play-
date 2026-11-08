#pragma once

#include <deque>
#include "../SoundHandler.h"
#ifndef __EMSCRIPTEN__
#include "openal/Device.h"
#include "openal/Context.h"
#include "openal/Source.h"
#include "openal/Buffer.h"
#endif

class CSH_OpenAL : public CSoundHandler
{
public:
	enum
	{
		MAX_BUFFERS = 64,
	};

	CSH_OpenAL();
	virtual ~CSH_OpenAL();

	static CSoundHandler* HandlerFactory();

	void Reset() override;
	void Write(int16*, unsigned int, unsigned int) override;
	bool HasFreeBuffers() override;
	void RecycleBuffers() override;

	uint32 GetFreeBufferCount() const;

private:
#ifndef __EMSCRIPTEN__
	typedef std::deque<ALuint> BufferList;

	OpenAl::CDevice m_device;
	OpenAl::CContext m_context;
	OpenAl::CSource m_source;

	BufferList m_availableBuffers;
	uint64 m_lastUpdateTime;
	bool m_mustSync;
	ALuint m_bufferNames[MAX_BUFFERS];
#endif
};
