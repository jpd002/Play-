#pragma once

#include "MIPS.h"
#include "ee/MA_VU.h"
#include "ee/VuExecutor.h"

class CTestVm
{
public:
						CTestVm();
	virtual				~CTestVm();

	void				Reset();
	void				ExecuteTest(uint32);

	CMIPS				m_cpu;
	CVuExecutor			m_executor;
	uint8*				m_vuMem = nullptr;
	uint8*				m_microMem = nullptr;
	CMA_VU				m_maVu;
};
