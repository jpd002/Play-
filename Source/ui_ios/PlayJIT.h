//
//  PlayJIT.h
//  Play! iOS - JIT Support for iOS 26+
//
//  Main interface for JIT memory allocation on iOS 26 with TXM
//  Uses BreakpointJIT framework + StikDebug for external JIT activation
//

#ifndef PlayJIT_h
#define PlayJIT_h

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// =============================================================================
	// Initialization
	// =============================================================================

	/**
	 * Initialize the JIT subsystem.
	 * Must be called early in app startup (e.g., applicationDidFinishLaunching).
	 *
	 * - Installs SIGTRAP handler to prevent crashes
	 * - Detects iOS version and TXM status
	 * - Checks if StikDebug activation is needed
	 *
	 * @return true if initialization successful
	 */
	bool PlayJIT_Initialize(void);

	/**
	 * Shutdown the JIT subsystem.
	 * Detaches debugger and cleans up resources.
	 */
	void PlayJIT_Shutdown(void);

	// =============================================================================
	// Status Checks
	// =============================================================================

	/**
	 * Check if running on iOS 26+ with TXM active.
	 * TXM (Thread Execution Manager) is present on A15+ and M2+ chips.
	 *
	 * @return true if TXM is active and external JIT activation required
	 */
	bool PlayJIT_HasTXM(void);

	/**
	 * Check if JIT is currently available.
	 * On pre-iOS 26: always true (MAP_JIT works)
	 * On iOS 26+ with TXM: true only if StikDebug has activated JIT
	 *
	 * @return true if JIT memory can be allocated
	 */
	bool PlayJIT_IsAvailable(void);

	/**
	 * Check if StikDebug activation is required.
	 *
	 * @return true if user needs to activate StikDebug first
	 */
	bool PlayJIT_NeedsActivation(void);

	/**
	 * Get iOS version as float (e.g., 26.0, 18.4)
	 */
	float PlayJIT_GetIOSVersion(void);

	// =============================================================================
	// StikDebug Activation (iOS 26+ only)
	// =============================================================================

	/**
	 * Request JIT activation via StikDebug app.
	 * Opens stikdebug:// URL scheme to activate JIT.
	 *
	 * @param completion Callback with success status (can be NULL)
	 *
	 * Usage:
	 *   PlayJIT_RequestActivation(^(bool success) {
	 *       if (success) {
	 *           NSLog(@"JIT activated!");
	 *       } else {
	 *           // Show error to user
	 *       }
	 *   });
	 */
	void PlayJIT_RequestActivation(void (^completion)(bool success));

	/**
	 * Wait for StikDebug to attach (blocking).
	 *
	 * @param timeout_ms Maximum time to wait in milliseconds
	 * @return true if debugger attached within timeout
	 */
	bool PlayJIT_WaitForActivation(uint32_t timeout_ms);

	// =============================================================================
	// Memory Allocation
	// =============================================================================

	/**
	 * Allocate dual-mapped JIT memory.
	 *
	 * Creates two views of the same physical memory:
	 * - rwPtr: Read-Write access for code generation
	 * - rxPtr: Read-Execute access for code execution
	 *
	 * @param size Size of memory region (will be page-aligned)
	 * @param rwPtr Output: Pointer for writing code
	 * @param rxPtr Output: Pointer for executing code
	 * @return true on success
	 *
	 * Usage:
	 *   void* rw = NULL;
	 *   void* rx = NULL;
	 *   if (PlayJIT_Allocate(4 * 1024 * 1024, &rw, &rx)) {
	 *       // Write code to rw
	 *       memcpy(rw, generatedCode, codeSize);
	 *       // Sync caches
	 *       PlayJIT_FlushCache(rx, codeSize);
	 *       // Execute from rx
	 *       ((void(*)(void))rx)();
	 *   }
	 */
	bool PlayJIT_Allocate(size_t size, void** rwPtr, void** rxPtr);

	/**
	 * Free previously allocated JIT memory.
	 *
	 * @param rwPtr Read-write pointer from PlayJIT_Allocate
	 * @param rxPtr Read-execute pointer from PlayJIT_Allocate
	 * @param size Size that was allocated
	 */
	void PlayJIT_Free(void* rwPtr, void* rxPtr, size_t size);

	/**
	 * Flush instruction cache after writing code.
	 * Must be called after writing to rwPtr before executing from rxPtr.
	 *
	 * @param addr Address to flush (rxPtr)
	 * @param size Size of modified region
	 */
	void PlayJIT_FlushCache(void* addr, size_t size);

	// =============================================================================
	// Advanced: Direct BreakpointJIT Access
	// =============================================================================

	/**
	 * Direct allocation via BreakpointJIT (for advanced use).
	 * Requires StikDebug to be attached.
	 *
	 * @param size Size of RX memory to allocate
	 * @return Pointer to executable memory, or NULL on failure
	 */
	void* PlayJIT_BreakAlloc(size_t size);

	/**
	 * Detach StikDebug debugger.
	 * Call after all JIT allocations are complete.
	 * CS_DEBUGGED flag remains set, JIT memory stays executable.
	 */
	void PlayJIT_BreakDetach(void);

#ifdef __cplusplus
}
#endif

#endif /* PlayJIT_h */
