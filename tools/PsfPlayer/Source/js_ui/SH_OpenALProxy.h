#pragma once

#include <emscripten/threading.h>
#include "../SH_OpenAL.h"

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
	em_queued_call* m_recycleBuffersCall = nullptr;
};
