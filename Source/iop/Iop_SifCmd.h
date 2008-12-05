#ifndef _IOP_SIFCMD_H_
#define _IOP_SIFCMD_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CSifCmd : public CModule
    {
    public:
                                CSifCmd(CIopBios&);
        virtual                 ~CSifCmd();

        virtual std::string     GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);

    private:
        CIopBios&               m_bios;

        void                    SifRegisterRpc(CMIPS&);
        void                    SifRpcLoop(uint32);
    };
}

#endif
