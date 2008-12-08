#ifndef _IOP_LIBSD_H_
#define _IOP_LIBSD_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"

namespace Iop
{
    class CLibSd : public CModule, public CSifModule
    {
    public:
                        CLibSd(CSifMan&);
        virtual         ~CLibSd();

        std::string     GetId() const;
        std::string     GetFunctionName(unsigned int) const;
        void            Invoke(CMIPS&, unsigned int);
        bool            Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

    private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000701
		};

        void            GetBufferSize(uint32*, uint32, uint32*, uint32);

    };
}

#endif
