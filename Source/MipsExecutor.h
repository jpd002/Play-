#pragma once

#include "Types.h"

class CMipsExecutor
{
public:
	virtual ~CMipsExecutor() = default;
	virtual void Reset() = 0;
	virtual int Execute(int) = 0;
	virtual void ClearActiveBlocksInRange(uint32 start, uint32 end) = 0;

#ifdef DEBUGGER_INCLUDED
	virtual bool MustBreak() const = 0;
	virtual void DisableBreakpointsOnce() = 0;
#endif
};
