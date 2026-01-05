//
//  BreakJIT_Extended.h
//  BreakpointJIT Extended
//
//  Extended API including TXM detection and CS_DEBUGGED checks
//  For use with Play! PS2 emulator iOS 26 JIT support
//

#ifndef BreakJIT_Extended_h
#define BreakJIT_Extended_h

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Core BreakpointJIT Functions (from Stossy11's framework)
// =============================================================================

/**
 * Request JIT memory allocation from StikDebug debugger.
 * @param addr Address of existing region (NULL for new allocation)
 * @param len Size of memory region in bytes
 * @return Pointer to RX memory region
 */
__attribute__((noinline, optnone, naked)) void* BreakGetJITMapping(void *addr, size_t len);

/**
 * Detach StikDebug debugger (CS_DEBUGGED remains set)
 */
__attribute__((noinline, optnone, naked)) void BreakJITDetach(void);

/**
 * Check JIT status via brk #0x69
 * @return 0xE0000069 if JIT active
 */
__attribute__((noinline, optnone, naked)) void* BreakMarkJITMapping(size_t bytes);

// =============================================================================
// Extended Helper Functions
// =============================================================================

/**
 * Check if running on iOS 26+ with TXM (Thread Execution Manager)
 * TXM is active on A15+ and M2+ chips with iOS 26+
 * @return true if TXM is active and external JIT activation required
 */
bool BreakHasTXM(void);

/**
 * Check if CS_DEBUGGED flag is set (debugger attached)
 * Uses csops() syscall to query process flags
 * @return true if CS_DEBUGGED (0x10000000) is set
 */
bool BreakIsDebugged(void);

/**
 * Check if JIT is currently active and usable
 * Combines TXM check and debugger status
 * @return true if JIT can be used
 */
bool BreakIsJITActive(void);

/**
 * Wait for debugger to attach with timeout
 * @param timeout_ms Timeout in milliseconds
 * @return true if debugger attached within timeout
 */
bool BreakWaitForDebugger(uint32_t timeout_ms);

/**
 * Install SIGTRAP handler to prevent crashes from breakpoints
 * Call this before any brk instructions if StikDebug might not be attached
 */
void BreakInstallTrapHandler(void);

/**
 * Get iOS version as float (e.g., 26.0, 18.4)
 * @return iOS version number
 */
float BreakGetIOSVersion(void);

/**
 * Check JIT active status via brk #0x69
 * @return true if brk #0x69 returns magic value 0xE0000069
 */
bool BreakCheckJITStatus(void);

// =============================================================================
// Dual Mapping Functions
// =============================================================================

/**
 * Allocate dual-mapped JIT memory (RW + RX views of same physical pages)
 * @param size Size of memory region
 * @param rwPtr Output: Pointer for read-write access (code generation)
 * @param rxPtr Output: Pointer for read-execute access (code execution)
 * @return true on success
 */
bool BreakAllocateDualMapping(size_t size, void** rwPtr, void** rxPtr);

/**
 * Free dual-mapped JIT memory
 * @param rwPtr Read-write pointer to free
 * @param rxPtr Read-execute pointer to free
 * @param size Size of memory region
 */
void BreakFreeDualMapping(void* rwPtr, void* rxPtr, size_t size);

/**
 * Invalidate instruction cache for JIT code
 * @param addr Start address
 * @param size Size of region
 */
void BreakInvalidateCache(void* addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* BreakJIT_Extended_h */
