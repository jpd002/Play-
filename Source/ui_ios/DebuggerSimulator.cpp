#include <dlfcn.h>
#include <sys/types.h>
#include <cstdlib>

#define PT_TRACE_ME 0
#define PT_DENY_ATTACH 31

typedef int (*ptrace_ptr_t)(int _request, pid_t _pid, caddr_t _addr, int _data);

void StartSimulateDebugger()
{
	auto ptrace_ptr = reinterpret_cast<ptrace_ptr_t>(dlsym(RTLD_SELF, "ptrace"));
	ptrace_ptr(PT_TRACE_ME, 0, NULL, 0);
}

void StopSimulateDebugger()
{
	auto ptrace_ptr = reinterpret_cast<ptrace_ptr_t>(dlsym(RTLD_SELF, "ptrace"));
	ptrace_ptr(PT_DENY_ATTACH, 0, NULL, 0);
}
