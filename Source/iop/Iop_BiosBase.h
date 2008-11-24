#ifndef _IOP_BIOSBASE_H_
#define _IOP_BIOSBASE_H_

#include "Types.h"

namespace Iop
{
    class CBiosBase
    {
    public:
        virtual         ~CBiosBase() {}
        virtual void    HandleException() = 0;
        virtual void    HandleInterrupt() = 0;
	    virtual void	CountTicks(uint32) = 0;

    private:

    };
}

#endif
