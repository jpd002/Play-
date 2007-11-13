#ifndef _IOP_THBASE_H_
#define _IOP_THBASE_H_

#include "Iop_Module.h"

namespace Iop
{
    class CThbase : public CModule
    {
    public:
                        CThbase();
        virtual         ~CThbase();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:

    };
}

#endif
