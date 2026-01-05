//
//  BreakpointJIT.h
//  BreakpointJIT Framework
//
//  iOS 26 JIT support via StikDebug breakpoint mechanism
//

#ifndef BREAKPOINTJIT_H
#define BREAKPOINTJIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Get JIT-enabled memory mapping via brk #0xf00d
/// This triggers the StikDebug debugger to prepare memory with RX permissions
/// @param size Size of memory region to allocate (optional, can be 0)
/// @return Pointer to RX-capable memory, or NULL on failure
void* BreakGetJITMapping(size_t size);

/// Mark a memory region as containing JIT code
/// Signals to the debugger that this region needs execute permissions
/// @param addr Address of the JIT code region
/// @param size Size of the memory region
void BreakMarkJITMapping(void* addr, size_t size);

/// Detach from the JIT debugging session
/// Call this when JIT is no longer needed to cleanly disconnect from StikDebug
void BreakJITDetach(void);

#ifdef __cplusplus
}
#endif

#endif /* BREAKPOINTJIT_H */
