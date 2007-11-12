#ifndef _IOP_LOADCORE_H_
#define _IOP_LOADCORE_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CLoadcore : public CModule
    {
    public:
                        CLoadcore(CIopBios&, uint8*);
        virtual         ~CLoadcore();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        uint32          RegisterLibraryEntries(uint32*);

        CIopBios&       m_bios;
        uint8*          m_ram;
    };
}

#endif
