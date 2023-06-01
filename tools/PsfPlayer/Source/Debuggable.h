#pragma once

#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include <functional>

class CDebuggable
{
public:
	typedef std::function<CMIPS&()> GetCpuFuncType;
	typedef std::function<void()> StepFuncType;

	GetCpuFuncType GetCpu;
	StepFuncType Step;

	CBiosDebugInfoProvider* biosDebugInfoProvider;
};
