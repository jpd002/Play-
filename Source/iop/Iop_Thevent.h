#ifndef _IOP_THEVENT_H_
#define _IOP_THEVENT_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CThevent : public CModule
    {
    public:
                        CThevent(CIopBios&, uint8*);
        virtual         ~CThevent();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        uint32          CreateEventFlag();

        uint8*          m_ram;
        CIopBios&       m_bios;
    };
}

#endif
