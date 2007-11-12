#ifndef _IOP_MODLOAD_H_
#define _IOP_MODLOAD_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CModload : public CModule
    {
    public:
                        CModload(CIopBios&, uint8*);
        virtual         ~CModload();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        uint32          LoadStartModule(const char*, uint32, const char*, uint32*);
        CIopBios&       m_bios;
        uint8*          m_ram;
    };
}

#endif
