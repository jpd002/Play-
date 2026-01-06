#pragma once

// Check if pthreads are enabled (Emscripten defines this when pthreads are enabled)
#if defined(__EMSCRIPTEN_PTHREADS__) || defined(EMSCRIPTEN_PTHREADS)
#include <emscripten/threading.h>
#define USE_PTHREADS 1
#else
#define USE_PTHREADS 0
// Forward declaration for em_queued_call when pthreads are disabled
typedef void* em_queued_call;
#endif

#include "SH_OpenAL.h"

class CSH_OpenALProxy : public CSoundHandler
{
public:
	CSH_OpenALProxy(CSH_OpenAL*);

	static FactoryFunction GetFactoryFunction(CSH_OpenAL*);

	void Reset() override;
	void Write(int16*, unsigned int, unsigned int) override;
	bool HasFreeBuffers() override;
	void RecycleBuffers() override;

private:
	CSH_OpenAL* m_parent = nullptr;
	unsigned int m_bufferCount = 0;
#if USE_PTHREADS
	em_queued_call* m_recycleBuffersCall = nullptr;
#else
	void* m_recycleBuffersCall = nullptr; // Not used when pthreads are disabled
#endif
};
