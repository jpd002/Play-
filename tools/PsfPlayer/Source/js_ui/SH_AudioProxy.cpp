#include "SH_AudioProxy.h"
#include <emscripten/threading.h>
#include <cstdio>

extern "C" void SoundHandlerResetProxy(CSoundHandler* handler)
{
    handler->Reset();
}

extern "C" void SoundHandlerWriteProxy(CSoundHandler* handler, int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
    handler->Write(samples, sampleCount, sampleRate);
    delete [] samples;
}

extern "C" int SoundHandlerHasFreeBuffersProxy(CSoundHandler* handler)
{
    return handler->HasFreeBuffers();
}

extern "C" void SoundHandlerRecycleBuffersProxy(CSoundHandler* handler)
{
    handler->RecycleBuffers();
}

CSH_AudioProxy::CSH_AudioProxy(CSoundHandler* parent)
: m_parent(parent)
{

}

CSoundHandler::FactoryFunction CSH_AudioProxy::GetFactoryFunction(CSoundHandler* soundHandler)
{
	return [soundHandler]() { return new CSH_AudioProxy(soundHandler); };
}

void CSH_AudioProxy::Reset()
{
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_RETURN_VALUE_V | 
        EM_FUNC_SIG_WITH_N_PARAMETERS(1) | 
        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I),
        &SoundHandlerResetProxy, m_parent);
}

void CSH_AudioProxy::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
    int16* proxySamples = new int16[sampleCount];
    memcpy(proxySamples, samples, sizeof(int16) * sampleCount);
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_RETURN_VALUE_V | 
        EM_FUNC_SIG_WITH_N_PARAMETERS(4) | 
        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I) | EM_FUNC_SIG_SET_PARAM(1, EM_FUNC_SIG_PARAM_I) | 
        EM_FUNC_SIG_SET_PARAM(2, EM_FUNC_SIG_PARAM_I) | EM_FUNC_SIG_SET_PARAM(3, EM_FUNC_SIG_PARAM_I),
        &SoundHandlerWriteProxy, m_parent, proxySamples, sampleCount, sampleRate);
}

bool CSH_AudioProxy::HasFreeBuffers()
{
//    int result = emscripten_sync_run_in_main_runtime_thread(
//        EM_FUNC_SIG_RETURN_VALUE_I |
//        EM_FUNC_SIG_WITH_N_PARAMETERS(1) |
//        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I),
//        &SoundHandlerHasFreeBuffersProxy, m_parent);
//    return result;
    return m_parent->HasFreeBuffers();
}

void CSH_AudioProxy::RecycleBuffers()
{
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_RETURN_VALUE_V | 
        EM_FUNC_SIG_WITH_N_PARAMETERS(1) | 
        EM_FUNC_SIG_SET_PARAM(0, EM_FUNC_SIG_PARAM_I),
        &SoundHandlerRecycleBuffersProxy, m_parent);
}
