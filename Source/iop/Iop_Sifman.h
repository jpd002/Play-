#ifndef _IOP_SIFMAN_H_
#define _IOP_SIFMAN_H_

#include "SifModule.h"

namespace Iop
{
    class CSifMan
    {
    public:
        virtual         ~CSifMan() {}

        virtual void    RegisterModule(uint32, CSifModule*) = 0;
        virtual void    SendPacket(void*, uint32) = 0;
    };
}

#endif
