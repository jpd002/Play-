#ifndef _IOP_MODULE_H_
#define _IOP_MODULE_H_

#include <string>
#include "../MIPS.h"

namespace Iop
{
    class CModule
    {
    public:
        virtual                 ~CModule() {}
        virtual std::string     GetId() const = 0;
		virtual std::string		GetFunctionName(unsigned int) const = 0;
        virtual void            Invoke(CMIPS&, unsigned int) = 0;

    private:

    };
};

#endif
