#ifndef _IOP_SIFMANPS2_H_
#define _IOP_SIFMANPS2_H_

#include "Iop_SifMan.h"
#include "./Sif.h"

namespace Iop
{
    class CSifManPs2 : public CSifMan
    {
    public:
                        CSifManPs2(CSIF&);
        virtual         ~CSifManPs2();

        virtual void    RegisterModule(CSifModule*);
        virtual void    SendPacket(void*, uint32);

    private:
        CSIF&           m_sif;
    };
}

#endif
