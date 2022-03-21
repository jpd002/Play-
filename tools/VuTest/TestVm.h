#pragma once

#include "AlignedAlloc.h"
#include "MIPS.h"
#include "ee/MA_VU.h"
#include "ee/VuExecutor.h"

class CTestVm
{
public:
	typedef std::vector<uint8> VuMemSnapshot;

	CTestVm();
	virtual ~CTestVm();

	void Reset();
	void ExecuteTest(uint32);

	uint32 IoPortWriteHandler(uint32 address, uint32 value);

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

	std::vector<VuMemSnapshot> m_xgKickSnapshots;
};
