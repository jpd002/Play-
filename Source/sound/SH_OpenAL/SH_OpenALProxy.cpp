#include "SH_OpenALProxy.h"
#include <cstdio>
#include <cassert>

CSH_OpenALProxy::CSH_OpenALProxy(CSH_OpenAL* parent)
    : m_parent(parent)
{
}

CSoundHandler::FactoryFunction CSH_OpenALProxy::GetFactoryFunction(CSH_OpenAL* soundHandler)
{
	return [soundHandler]() { return new CSH_OpenALProxy(soundHandler); };
}

void CSH_OpenALProxy::Reset()
{
	if(m_recycleBuffersCall != nullptr)
	{
		emscripten_async_waitable_close(m_recycleBuffersCall);
		m_recycleBuffersCall = nullptr;
	}
	if(m_parent)
	{
		m_parent->Reset();
	}
	m_bufferCount = CSH_OpenAL::MAX_BUFFERS;
}

void CSH_OpenALProxy::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	if(m_parent)
	{
		m_parent->Write(samples, sampleCount, sampleRate);
	}
}

bool CSH_OpenALProxy::HasFreeBuffers()
{
	return true;
}

void CSH_OpenALProxy::RecycleBuffers()
{
}
