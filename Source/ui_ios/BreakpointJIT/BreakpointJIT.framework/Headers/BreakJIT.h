//
//  BreakJIT.h
//  BreakpointJIT
//
//  Created by Stossy11 on 09/07/2025.
//

#ifndef BreakGetJITMapping_h
#define BreakGetJITMapping_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request JIT memory allocation from StikDebug debugger.
 * When addr is NULL, allocates new RX memory of specified size.
 * When addr is non-NULL, prepares an existing region.
 * 
 * @param addr Address of existing region (NULL for new allocation)
 * @param len Size of memory region in bytes
 * @return Pointer to RX memory region, or NULL on failure
 * 
 * Internally: Sets x16=1 (CMD_PREPARE_REGION), triggers brk #0xf00d
 * StikDebug intercepts and allocates memory via GDB protocol.
 */
__attribute__((noinline, optnone, naked)) void* BreakGetJITMapping(void *addr, size_t len);

/**
 * Detach the StikDebug debugger after JIT setup is complete.
 * CS_DEBUGGED flag remains set, allowing JIT execution to continue.
 * 
 * Internally: Sets x16=0 (CMD_DETACH), triggers brk #0xf00d
 */
__attribute__((noinline, optnone, naked)) void BreakJITDetach(void);

/**
 * Allocate new JIT memory region of specified size.
 * Equivalent to BreakGetJITMapping(NULL, bytes) but with single parameter.
 * 
 * @param bytes Size of memory region to allocate
 * @return Pointer to RX memory region, or NULL on failure
 * 
 * Note: On iOS 26+, this triggers brk #0x69 for JIT status check,
 * actual allocation may require BreakGetJITMapping.
 */
__attribute__((noinline, optnone, naked)) void* BreakMarkJITMapping(size_t bytes);

#ifdef __cplusplus
}
#endif

#endif /* BreakGetJITMapping_h */
