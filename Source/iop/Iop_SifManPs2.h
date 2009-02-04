#ifndef _IOP_SIFMANPS2_H_
#define _IOP_SIFMANPS2_H_

#include "Iop_SifMan.h"
#include "../Sif.h"

namespace Iop
{
    class CSifManPs2 : public CSifMan
    {
    public:
                        CSifManPs2(CSIF&);
        virtual         ~CSifManPs2();

        virtual void    RegisterModule(uint32, CSifModule*);
        virtual void    UnregisterModule(uint32);
        virtual void    SendPacket(void*, uint32);
        virtual void    SetDmaBuffer(uint32, uint32);
        virtual void    SendCallReply(uint32, void*);

    private:
        CSIF&           m_sif;
    };
}

#endif
