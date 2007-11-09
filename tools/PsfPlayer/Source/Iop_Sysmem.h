#ifndef _IOP_SYSMEM_H_
#define _IOP_SYSMEM_H_

#include "Iop_Module.h"

namespace Iop
{
    class CSysmem : public CModule
    {
    public:
                        CSysmem();
        virtual         ~CSysmem();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:

    };
}


#endif
