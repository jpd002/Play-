//
//  PlayJIT.mm
//  Play! iOS - JIT Support for iOS 26+
//
//  Implementation using BreakpointJIT framework + StikDebug
//

#import "PlayJIT.h"
#import <BreakpointJIT/BreakJIT.h>

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <mach/mach.h>
#import <mach/vm_map.h>
#import <sys/mman.h>
#import <sys/sysctl.h>
#import <signal.h>
#import <dlfcn.h>
#import <libkern/OSCacheControl.h>

// =============================================================================
// Constants
// =============================================================================

#define CS_DEBUGGED 0x10000000
#define SYS_csops 169
#define JIT_ACTIVE_MAGIC 0xE0000069ULL

// =============================================================================
// Private State
// =============================================================================

static bool g_initialized = false;
static bool g_hasTXM = false;
static float g_iosVersion = 0.0f;
static struct sigaction g_oldTrapAction;
static bool g_trapHandlerInstalled = false;

// =============================================================================
// Private: csops syscall
// =============================================================================

static int csops(pid_t pid, unsigned int ops, void* useraddr, size_t usersize)
{
	return syscall(SYS_csops, pid, ops, useraddr, usersize);
}

static bool isDebugged(void)
{
	uint32_t flags = 0;
	if(csops(getpid(), 0, &flags, sizeof(flags)) == 0)
	{
		return (flags & CS_DEBUGGED) != 0;
	}
	return false;
}

// =============================================================================
// Private: SIGTRAP Handler
// =============================================================================

static void trapHandler(int sig, siginfo_t* info, void* context)
{
	// Ignore SIGTRAP from brk instructions when debugger not attached
	// This prevents crashes if StikDebug isn't running
	(void)sig;
	(void)info;
	(void)context;
	NSLog(@"[PlayJIT] SIGTRAP received - StikDebug may not be attached");
}

static void installTrapHandler(void)
{
	if(g_trapHandlerInstalled) return;

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = trapHandler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);

	if(sigaction(SIGTRAP, &action, &g_oldTrapAction) == 0)
	{
		g_trapHandlerInstalled = true;
		NSLog(@"[PlayJIT] SIGTRAP handler installed");
	}
}

// =============================================================================
// Private: Version Detection
// =============================================================================

static float detectIOSVersion(void)
{
	char version[32] = {0};
	size_t size = sizeof(version);

	if(sysctlbyname("kern.osproductversion", version, &size, NULL, 0) == 0)
	{
		return strtof(version, NULL);
	}
	return 0.0f;
}

static bool detectTXM(void)
{
	// Check environment variable (set by StikDebug or for testing)
	const char* env = getenv("HAS_TXM");
	if(env && strcmp(env, "1") == 0)
	{
		return true;
	}

	// iOS 26+ required for TXM
	if(g_iosVersion < 26.0f)
	{
		return false;
	}

	// Check chip (A15+ / M2+ have TXM)
	uint32_t cpufamily = 0;
	size_t size = sizeof(cpufamily);
	if(sysctlbyname("hw.cpufamily", &cpufamily, &size, NULL, 0) == 0)
	{
		switch(cpufamily)
		{
		case 0xDA33D83D: // A15 Bionic
		case 0x8765EDEA: // A16 Bionic
		case 0xFA33415E: // A17 Pro
		case 0x5F4DEA93: // A18
		case 0x72015832: // A18 Pro
		case 0x6F5129AC: // M2
		case 0xDC6E3A2A: // M3
		case 0x041A314C: // M4
			return true;
		default:
			break;
		}
	}

	// Fallback: Try MAP_JIT to detect if TXM blocks it
	void* test = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
	                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
	if(test == MAP_FAILED)
	{
		return true; // TXM is blocking JIT
	}
	munmap(test, 4096);
	return false;
}

// =============================================================================
// Initialization
// =============================================================================

bool PlayJIT_Initialize(void)
{
	if(g_initialized) return true;

	NSLog(@"[PlayJIT] Initializing...");

	// Detect iOS version
	g_iosVersion = detectIOSVersion();
	NSLog(@"[PlayJIT] iOS version: %.1f", g_iosVersion);

	// Detect TXM
	g_hasTXM = detectTXM();
	NSLog(@"[PlayJIT] TXM active: %@", g_hasTXM ? @"YES" : @"NO");

	// Install trap handler (prevents crashes if StikDebug not attached)
	installTrapHandler();

	// Check current status
	if(g_hasTXM)
	{
		bool debugged = isDebugged();
		NSLog(@"[PlayJIT] Debugger attached: %@", debugged ? @"YES" : @"NO");

		if(!debugged)
		{
			NSLog(@"[PlayJIT] StikDebug activation required for JIT");
		}
	}
	else
	{
		NSLog(@"[PlayJIT] No TXM - JIT available via MAP_JIT");
	}

	g_initialized = true;
	return true;
}

void PlayJIT_Shutdown(void)
{
	if(!g_initialized) return;

	NSLog(@"[PlayJIT] Shutting down...");

	// Detach debugger if attached
	if(g_hasTXM && isDebugged())
	{
		BreakJITDetach();
		NSLog(@"[PlayJIT] Debugger detached");
	}

	g_initialized = false;
}

// =============================================================================
// Status Checks
// =============================================================================

bool PlayJIT_HasTXM(void)
{
	return g_hasTXM;
}

bool PlayJIT_IsAvailable(void)
{
	if(!g_hasTXM)
	{
		return true; // Pre-TXM: always available
	}
	return isDebugged(); // TXM: need debugger
}

bool PlayJIT_NeedsActivation(void)
{
	return g_hasTXM && !isDebugged();
}

float PlayJIT_GetIOSVersion(void)
{
	return g_iosVersion;
}

// =============================================================================
// StikDebug Activation
// =============================================================================

// Embedded JavaScript for StikDebug protocol
static NSString* createJITScript(void)
{
	return @"\"use strict\";\n"
	        "const CMD_DETACH = 0;\n"
	        "const CMD_PREPARE_REGION = 1;\n"
	        "\n"
	        "function numberToLittleEndianHexString(num) {\n"
	        "    let hex = '';\n"
	        "    for (let i = 0; i < 8; i++) {\n"
	        "        hex += (Number((num >> BigInt(i * 8)) & 0xFFn)).toString(16).padStart(2, '0');\n"
	        "    }\n"
	        "    return hex;\n"
	        "}\n"
	        "\n"
	        "function JIT26PrepareRegion(brkResponse) {\n"
	        "    const registers = brkResponse.split(';');\n"
	        "    let x0 = 0n, x1 = 0n, tid = '';\n"
	        "    for (const reg of registers) {\n"
	        "        if (reg.startsWith('0:')) x0 = BigInt('0x' + reg.slice(2).match(/../g).reverse().join(''));\n"
	        "        if (reg.startsWith('1:')) x1 = BigInt('0x' + reg.slice(2).match(/../g).reverse().join(''));\n"
	        "        if (reg.startsWith('thread:')) tid = reg.slice(7);\n"
	        "    }\n"
	        "    let jitPageAddress = x0;\n"
	        "    if (x0 == 0n) {\n"
	        "        const rxResponse = send_command(`_M${x1.toString(16)},rx`);\n"
	        "        jitPageAddress = BigInt(`0x${rxResponse}`);\n"
	        "    }\n"
	        "    prepare_memory_region(jitPageAddress, x1);\n"
	        "    send_command(`P0=${numberToLittleEndianHexString(jitPageAddress)};thread:${tid};`);\n"
	        "}\n"
	        "\n"
	        "function onBreakpoint(brkResponse) {\n"
	        "    const registers = brkResponse.split(';');\n"
	        "    let x16 = 0n;\n"
	        "    for (const reg of registers) {\n"
	        "        if (reg.startsWith('10:')) x16 = BigInt('0x' + reg.slice(3).match(/../g).reverse().join(''));\n"
	        "    }\n"
	        "    if (x16 == BigInt(CMD_DETACH)) {\n"
	        "        send_command('D');\n"
	        "        return;\n"
	        "    }\n"
	        "    if (x16 == BigInt(CMD_PREPARE_REGION)) {\n"
	        "        JIT26PrepareRegion(brkResponse);\n"
	        "    }\n"
	        "    send_command('c');\n"
	        "}\n";
}

void PlayJIT_RequestActivation(void (^completion)(bool success))
{
	if(!g_hasTXM)
	{
		NSLog(@"[PlayJIT] No TXM - activation not needed");
		if(completion) completion(true);
		return;
	}

	if(isDebugged())
	{
		NSLog(@"[PlayJIT] Already activated");
		if(completion) completion(true);
		return;
	}

	NSString* bundleID = [[NSBundle mainBundle] bundleIdentifier] ?: @"com.play.Play";
	pid_t pid = getpid();
	NSString* script = createJITScript();
	NSData* scriptData = [script dataUsingEncoding:NSUTF8StringEncoding];
	NSString* scriptBase64 = [scriptData base64EncodedStringWithOptions:0];

	// URL encode the base64 (replace + with %2B, / with %2F, = with %3D)
	scriptBase64 = [scriptBase64 stringByAddingPercentEncodingWithAllowedCharacters:
	                                 [NSCharacterSet URLQueryAllowedCharacterSet]];

	NSString* urlString = [NSString stringWithFormat:
	                                    @"stikdebug://enable-jit?bundle-id=%@&pid=%d&script-name=Play&script-data=%@",
	                                    bundleID, pid, scriptBase64];

	NSURL* url = [NSURL URLWithString:urlString];

	NSLog(@"[PlayJIT] Opening StikDebug URL...");

	dispatch_async(dispatch_get_main_queue(), ^{
	  [[UIApplication sharedApplication] openURL:url
		  options:@{}
		  completionHandler:^(BOOL opened) {
			if(!opened)
			{
				NSLog(@"[PlayJIT] Failed to open StikDebug - is it installed?");
				if(completion) completion(false);
				return;
			}

			NSLog(@"[PlayJIT] StikDebug opened, waiting for activation...");

			// Wait for debugger to attach
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			  bool attached = PlayJIT_WaitForActivation(5000);

			  dispatch_async(dispatch_get_main_queue(), ^{
				if(attached)
				{
					NSLog(@"[PlayJIT] JIT activated successfully!");
				}
				else
				{
					NSLog(@"[PlayJIT] Activation timeout - debugger not attached");
				}
				if(completion) completion(attached);
			  });
			});
		  }];
	});
}

bool PlayJIT_WaitForActivation(uint32_t timeout_ms)
{
	uint32_t elapsed = 0;
	const uint32_t interval = 50;

	while(elapsed < timeout_ms)
	{
		if(isDebugged())
		{
			return true;
		}
		usleep(interval * 1000);
		elapsed += interval;
	}
	return isDebugged();
}

// =============================================================================
// Memory Allocation
// =============================================================================

bool PlayJIT_Allocate(size_t size, void** rwPtr, void** rxPtr)
{
	if(!rwPtr || !rxPtr || size == 0)
	{
		NSLog(@"[PlayJIT] Invalid allocation parameters");
		return false;
	}

	// Align to page size
	size_t pageSize = getpagesize();
	size_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

	void* rxBase = NULL;

	if(g_hasTXM)
	{
		// iOS 26+ with TXM: Use BreakpointJIT
		if(!isDebugged())
		{
			NSLog(@"[PlayJIT] Cannot allocate - debugger not attached");
			return false;
		}

		NSLog(@"[PlayJIT] Allocating %zu bytes via BreakpointJIT", alignedSize);
		rxBase = BreakGetJITMapping(NULL, alignedSize);

		if(!rxBase)
		{
			NSLog(@"[PlayJIT] BreakGetJITMapping failed");
			return false;
		}
	}
	else
	{
		// Pre-TXM: Standard mmap with MAP_JIT
		NSLog(@"[PlayJIT] Allocating %zu bytes via mmap+MAP_JIT", alignedSize);
		rxBase = mmap(NULL, alignedSize,
		              PROT_READ | PROT_EXEC,
		              MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
		              -1, 0);

		if(rxBase == MAP_FAILED)
		{
			NSLog(@"[PlayJIT] mmap failed: %s", strerror(errno));
			return false;
		}
	}

	// Create RW mapping via vm_remap
	vm_address_t rwBase = 0;
	vm_prot_t curProt, maxProt;

	kern_return_t kr = vm_remap(
	    mach_task_self(),
	    &rwBase,
	    alignedSize,
	    0,
	    VM_FLAGS_ANYWHERE,
	    mach_task_self(),
	    (vm_address_t)rxBase,
	    FALSE,
	    &curProt,
	    &maxProt,
	    VM_INHERIT_NONE);

	if(kr != KERN_SUCCESS)
	{
		NSLog(@"[PlayJIT] vm_remap failed: %d", kr);
		if(!g_hasTXM)
		{
			munmap(rxBase, alignedSize);
		}
		return false;
	}

	// Set RW protection on the alias
	kr = vm_protect(mach_task_self(), rwBase, alignedSize, FALSE,
	                VM_PROT_READ | VM_PROT_WRITE);

	if(kr != KERN_SUCCESS)
	{
		NSLog(@"[PlayJIT] vm_protect failed: %d", kr);
		vm_deallocate(mach_task_self(), rwBase, alignedSize);
		if(!g_hasTXM)
		{
			munmap(rxBase, alignedSize);
		}
		return false;
	}

	*rwPtr = (void*)rwBase;
	*rxPtr = rxBase;

	NSLog(@"[PlayJIT] Allocated: RW=%p RX=%p size=%zu", *rwPtr, *rxPtr, alignedSize);
	return true;
}

void PlayJIT_Free(void* rwPtr, void* rxPtr, size_t size)
{
	size_t pageSize = getpagesize();
	size_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

	if(rwPtr)
	{
		vm_deallocate(mach_task_self(), (vm_address_t)rwPtr, alignedSize);
	}

	if(rxPtr && !g_hasTXM)
	{
		munmap(rxPtr, alignedSize);
	}

	NSLog(@"[PlayJIT] Freed: RW=%p RX=%p size=%zu", rwPtr, rxPtr, alignedSize);
}

void PlayJIT_FlushCache(void* addr, size_t size)
{
	sys_icache_invalidate(addr, size);
}

// =============================================================================
// Direct BreakpointJIT Access
// =============================================================================

void* PlayJIT_BreakAlloc(size_t size)
{
	return BreakGetJITMapping(NULL, size);
}

void PlayJIT_BreakDetach(void)
{
	BreakJITDetach();
	NSLog(@"[PlayJIT] BreakJITDetach called");
}
