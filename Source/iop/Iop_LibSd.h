#ifndef _IOP_LIBSD_H_
#define _IOP_LIBSD_H_

#include "Iop_Module.h"
#include "../SIF.h"

namespace Iop
{
    class CLibSd : public CModule, public CSifModule
    {
    public:
                        CLibSd(CSIF&);
        virtual         ~CLibSd();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);
        void            Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000701
		};

        void            GetBufferSize(uint32*, uint32, uint32*, uint32);

    };
}

#endif
