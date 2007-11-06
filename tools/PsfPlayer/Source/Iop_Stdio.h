#ifndef _IOP_STDIO_H_
#define _IOP_STDIO_H_

#include "Iop_Module.h"

namespace Iop
{
    class CStdio : public CModule
    {
    public:
                        CStdio();
        virtual         ~CStdio();
        
        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        void            Printf(CMIPS&);
    };
}

#endif
