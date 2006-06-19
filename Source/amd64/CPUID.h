#ifndef _CPUID_H_
#define _CPUID_H_

#include "Types.h"

extern "C" uint32			_cpuid_GetCpuFeatures();
extern "C" void				_cpuid_GetCpuIdString(char*);

#endif
