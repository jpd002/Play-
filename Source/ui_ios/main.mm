#import <UIKit/UIKit.h>
#import <dlfcn.h>
#import "AppDelegate.h"

#define PT_TRACE_ME 0
typedef int (*ptrace_ptr_t)(int _request, pid_t _pid, caddr_t _addr, int _data);

static void SimulateDebugger()
{
	auto ptrace_ptr = reinterpret_cast<ptrace_ptr_t>(dlsym(RTLD_SELF, "ptrace"));
	ptrace_ptr(PT_TRACE_ME, 0, NULL, 0);
}

int main(int argc, char * argv[])
{
	SimulateDebugger();
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
