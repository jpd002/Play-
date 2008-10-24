#ifndef _IOP_INTC_H_
#define _IOP_INTC_H_

#include "Types.h"

namespace Iop
{
    class CIntc
    {
    public:
        enum LINES
        {
            LINE_VBLANK     = 0x00,
            LINE_SBUS       = 0x01,
            LINE_CDROM      = 0x02,
            LINE_RTC0       = 0x04,
            LINE_RTC1       = 0x05,
            LINE_RTC2       = 0x06,
            LINE_SPU2       = 0x09,
            LINE_EVBLANK    = 0x0B,
            LINE_DEV9       = 0x0D,
            LINE_RTC3       = 0x0E,
            LINE_RTC4       = 0x0F,
            LINE_RTC5       = 0x10,
            LINE_SIO2       = 0x11,
            LINE_USB        = 0x16,
            LINE_ILINK      = 0x18,

            LINE_DMA2       = 0x22,
            LINE_DMA4       = 0x24,     //spu2 core0
            LINE_DMA8       = 0x28,     //spu2 core1
            LINE_DMA_DEV9   = 0x29,
            LINE_DMA9       = 0x2A,     //sif0
            LINE_DMA10      = 0x2B,     //sif1
        };
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
