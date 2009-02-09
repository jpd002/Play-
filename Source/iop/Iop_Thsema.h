#ifndef _IOP_THSEMA_H_
#define _IOP_THSEMA_H_

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
    class CThsema : public CModule
    {
    public:
                        CThsema(CIopBios&, uint8*);
        virtual         ~CThsema();

        std::string     GetId() const;
		std::string		GetFunctionName(unsigned int) const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        struct SEMAPHORE
        {
            uint32      attributes;
            uint32      options;
            uint32      initialCount;
            uint32      maxCount;
        };

        uint32          CreateSemaphore(const SEMAPHORE*);
        uint32          DeleteSemaphore(uint32);
        uint32          WaitSemaphore(uint32);
        uint32          SignalSemaphore(uint32);
		uint32			iSignalSemaphore(uint32);

        uint8*          m_ram;
        CIopBios&       m_bios;
    };
}

#endif
