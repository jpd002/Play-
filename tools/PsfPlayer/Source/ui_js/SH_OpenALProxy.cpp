#include "SH_OpenALProxy.h"
#include <cstdio>
#include <cassert>

extern "C" void SH_OpenAL_Reset_Proxy(CSH_OpenAL* handler)
{
	handler->Reset();
}

extern "C" void SH_OpenAL_Write_Proxy(CSH_OpenAL* handler, int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	handler->Write(samples, sampleCount, sampleRate);
	delete[] samples;
}

extern "C" int SH_OpenAL_RecycleBuffers_Proxy(CSH_OpenAL* handler)
{
	handler->RecycleBuffers();
	return handler->GetFreeBufferCount();
}

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
	emscripten_async_run_in_main_runtime_thread(
	    EM_FUNC_SIG_RETURN_VALUE_V |
	        EM_FUNC_SIG_WITH_N_PARAMETERS(1) |
	        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I),
	    &SH_OpenAL_Reset_Proxy, m_parent);
	m_bufferCount = CSH_OpenAL::MAX_BUFFERS;
}

void CSH_OpenALProxy::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	assert(m_bufferCount != 0);
	if(m_bufferCount == 0) return;
	int16* proxySamples = new int16[sampleCount];
	memcpy(proxySamples, samples, sizeof(int16) * sampleCount);
	emscripten_async_run_in_main_runtime_thread(
	    EM_FUNC_SIG_RETURN_VALUE_V |
	        EM_FUNC_SIG_WITH_N_PARAMETERS(4) |
	        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I) | EM_FUNC_SIG_SET_PARAM(1, EM_FUNC_SIG_PARAM_I) |
	        EM_FUNC_SIG_SET_PARAM(2, EM_FUNC_SIG_PARAM_I) | EM_FUNC_SIG_SET_PARAM(3, EM_FUNC_SIG_PARAM_I),
	    &SH_OpenAL_Write_Proxy, m_parent, proxySamples, sampleCount, sampleRate);
	m_bufferCount--;
}

bool CSH_OpenALProxy::HasFreeBuffers()
{
	return (m_bufferCount != 0);
}

void CSH_OpenALProxy::RecycleBuffers()
{
	if(m_recycleBuffersCall != nullptr)
	{
		int bufferCount = 0;
		EMSCRIPTEN_RESULT result = emscripten_wait_for_call_i(m_recycleBuffersCall, 0, &bufferCount);
		if(result == EMSCRIPTEN_RESULT_TIMED_OUT) return;
		assert(result == EMSCRIPTEN_RESULT_SUCCESS);
		m_bufferCount = bufferCount;
		emscripten_async_waitable_close(m_recycleBuffersCall);
		m_recycleBuffersCall = nullptr;
	}
	else
	{
		m_recycleBuffersCall = emscripten_async_waitable_run_in_main_runtime_thread(
		    EM_FUNC_SIG_RETURN_VALUE_I |
		        EM_FUNC_SIG_WITH_N_PARAMETERS(1) |
		        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I),
		    &SH_OpenAL_RecycleBuffers_Proxy, m_parent);
	}
}
