#ifndef _DEBUGGABLE_H_
#define _DEBUGGABLE_H_

#include "MIPS.h"
#include "MIPSModule.h"
#include <list>
#include <functional>

class CDebuggable
{
public:
	typedef std::list<MIPSMODULE> ModuleList;

    typedef std::tr1::function<CMIPS& ()> GetCpuFuncType;
    typedef std::tr1::function<void ()> StepFuncType;
	typedef std::tr1::function<ModuleList ()> GetModulesFuncType;

    GetCpuFuncType      GetCpu;
    StepFuncType        Step;
	GetModulesFuncType	GetModules;
};

#endif
