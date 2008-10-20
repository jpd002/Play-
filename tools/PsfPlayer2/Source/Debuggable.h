#ifndef _DEBUGGABLE_H_
#define _DEBUGGABLE_H_

#include "MIPS.h"
#include <functional>

class CDebuggable
{
public:
    typedef std::tr1::function<CMIPS& ()> GetCpuFuncType;
    typedef std::tr1::function<void ()> StepFuncType;

    GetCpuFuncType      GetCpu;
    StepFuncType        Step;
};

#endif
