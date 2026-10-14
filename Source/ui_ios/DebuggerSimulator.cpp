#include <dlfcn.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#define PT_TRACE_ME 0
#define PT_DENY_ATTACH 31

typedef int (*ptrace_ptr_t)(int _request, pid_t _pid, caddr_t _addr, int _data);

// True if a debugger is already attached to this process (e.g. StikDebug, or
// LiveContainer's JIT enabler). In that case JIT is already set up.
static bool IsAlreadyBeingTraced()
{
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
	struct kinfo_proc info;
	memset(&info, 0, sizeof(info));
	size_t size = sizeof(info);
	if(sysctl(mib, 4, &info, &size, nullptr, 0) != 0) return false;
	return (info.kp_proc.p_flag & P_TRACED) != 0;
}

void StartSimulateDebugger()
{
	// Play!'s legacy self-"debugger" trick to enable JIT. On modern iOS this is
	// unnecessary and harmful: JIT comes from an attached debugger (StikDebug) or
	// LiveContainer's enabler, and self-tracing can clobber that JIT/codesigning
	// state. Skip it whenever a debugger is already attached.
	if(IsAlreadyBeingTraced()) return;
	auto ptrace_ptr = reinterpret_cast<ptrace_ptr_t>(dlsym(RTLD_SELF, "ptrace"));
	if(ptrace_ptr) ptrace_ptr(PT_TRACE_ME, 0, NULL, 0);
}

void StopSimulateDebugger()
{
	auto ptrace_ptr = reinterpret_cast<ptrace_ptr_t>(dlsym(RTLD_SELF, "ptrace"));
	ptrace_ptr(PT_DENY_ATTACH, 0, NULL, 0);
}
