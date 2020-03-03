#pragma once

#include "AlignedAlloc.h"
#include "MIPS.h"
#include "ee/MA_VU.h"
#include "ee/VuExecutor.h"

class CTestVm
{
public:
	CTestVm();
	virtual ~CTestVm();

	void Reset();
	void ExecuteTest(uint32);

	void* operator new(size_t allocSize)
	{
		return framework_aligned_alloc(allocSize, 0x10);
	}

	void operator delete(void* ptr)
	{
		return framework_aligned_free(ptr);
	}

	CMIPS m_cpu;
	CVuExecutor m_executor;
	uint8* m_vuMem = nullptr;
	uint8* m_microMem = nullptr;
	CMA_VU m_maVu;
};
