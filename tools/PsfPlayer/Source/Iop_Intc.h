#ifndef _IOP_INTC_H_
#define _IOP_INTC_H_

#include "Types.h"

namespace Iop
{
    class CIntc
    {
    public:
                    CIntc();
        virtual     ~CIntc();

        void        AssertLine(unsigned int);
        void        ClearLine(unsigned int);
        void        SetStatus(uint32);
        void        SetMask(uint32);
        bool        HasPendingInterrupt();

    private:
        uint32      m_status;
        uint32      m_mask;
    };
}

#endif
