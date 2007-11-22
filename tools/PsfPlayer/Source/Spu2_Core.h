#ifndef _SPU2_CORE_H_
#define _SPU2_CORE_H_

#include "Types.h"

namespace Spu2
{
    class CCore
    {
    public:
                    CCore(unsigned int, uint32);
        virtual     ~CCore();

        uint16      ReadRegister(uint32);
        void        WriteRegister(uint32, uint16);

        enum REGISTERS
        {
            STATX = 0x344,
        };

    private:
        void            LogRead(uint32);
        uint32          m_baseAddress;
        unsigned int    m_coreId;
    };
};

#endif
