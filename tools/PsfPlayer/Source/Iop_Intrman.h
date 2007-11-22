#ifndef _IOP_INTRMAN_H_
#define _IOP_INTRMAN_H_

#include "Iop_Module.h"

namespace Iop
{
    class CIntrman : public CModule
    {
    public:
                        CIntrman();
        virtual         ~CIntrman();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        
    };
}

#endif

