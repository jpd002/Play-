#ifndef _IOP_INTRMAN_H_
#define _IOP_INTRMAN_H_

#include "Iop_Module.h"

namespace Iop
{
    class CIntrman : public CModule
    {
    public:
                        CIntrman(uint8*);
        virtual         ~CIntrman();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        uint32          EnableInterrupts(CMIPS&);
        uint32          SuspendInterrupts(CMIPS&, uint32*);
        uint32          ResumeInterrupts(CMIPS&, uint32);
        uint32          QueryIntrContext(CMIPS&);
        uint8*          m_ram;
    };
}

#endif

