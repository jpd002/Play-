#ifndef _IOP_SIFCMD_H_
#define _IOP_SIFCMD_H_

#include "Iop_Module.h"

namespace Iop
{
    class CSifCmd : public CModule
    {
    public:
                                CSifCmd();
        virtual                 ~CSifCmd();

        virtual std::string     GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);

    private:
        void                    SifRegisterRpc(CMIPS&);
    };
}

#endif
