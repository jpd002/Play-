#ifndef _IOP_SIFMAN_H_
#define _IOP_SIFMAN_H_

#include "../SifModule.h"
#include "Iop_Module.h"

namespace Iop
{
	class CSifMan : public CModule
    {
    public:
								CSifMan();
        virtual					~CSifMan();

        virtual std::string     GetId() const;
        virtual void            Invoke(CMIPS&, unsigned int);

		virtual void			RegisterModule(uint32, CSifModule*) = 0;
        virtual void			SendPacket(void*, uint32) = 0;
        virtual void			SetDmaBuffer(uint8*, uint32) = 0;

	private:
		uint32					SifSetDma(uint32, uint32);
    };
}

#endif
