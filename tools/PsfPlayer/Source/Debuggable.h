#ifndef _DEBUGGABLE_H_
#define _DEBUGGABLE_H_

#include "BiosDebugInfoProvider.h"
#include "MIPS.h"
#include <functional>

class CDebuggable
{
public:
	typedef std::function<CMIPS&()> GetCpuFuncType;
	typedef std::function<void()>   StepFuncType;

	GetCpuFuncType GetCpu;
	StepFuncType   Step;

	CBiosDebugInfoProvider* biosDebugInfoProvider;
};

#endif
