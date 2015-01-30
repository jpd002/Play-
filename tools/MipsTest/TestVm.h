#pragma once

#include "MIPS.h"
#include "MA_EE.h"
#include "MipsExecutor.h"

class CTestVm
{
public:
	enum RAM_SIZE
	{
		RAM_SIZE = 0x200000,
	};

						CTestVm();
	virtual				~CTestVm();

	void				Reset();
	void				ExecuteTest(uint32);

	uint8*				m_ram;
	CMIPS				m_cpu;
	CMA_EE				m_cpuArch;
	CMipsExecutor		m_executor;
};
