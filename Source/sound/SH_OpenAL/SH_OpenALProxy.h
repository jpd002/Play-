#pragma once

// Check if pthreads are enabled
// When -sUSE_PTHREADS=0 is set, Emscripten won't define these macros
// If USE_PTHREADS is explicitly defined via compiler flags, use that value
#ifndef USE_PTHREADS
	#if defined(__EMSCRIPTEN_PTHREADS__) || defined(EMSCRIPTEN_PTHREADS) || defined(USE_PTHREADS_ENABLED)
		#define USE_PTHREADS 1
		#include <emscripten/threading.h>
	#else
		// Pthreads are disabled - don't include threading.h and use direct calls
		#define USE_PTHREADS 0
		// Forward declaration for em_queued_call when pthreads are disabled
		typedef void* em_queued_call;
	#endif
#else
	// USE_PTHREADS was already defined (e.g., via compiler flags)
	// Only include threading.h if it's enabled
	#if USE_PTHREADS == 1
		#include <emscripten/threading.h>
	#else
		// Forward declaration for em_queued_call when pthreads are disabled
		typedef void* em_queued_call;
	#endif
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
