#ifndef _IOP_VBLANK_H_
#define _IOP_VBLANK_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CVblank : public CModule
    {
    public:
                                CVblank(CIopBios&);
        virtual                 ~CVblank();
        virtual std::string     GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);

    private:
        void                    WaitVblankStart();
        void                    WaitVblankEnd();
		uint32					RegisterVblankHandler(CMIPS&, uint32, uint32, uint32, uint32);

        CIopBios&               m_bios;
    };
}

#endif
