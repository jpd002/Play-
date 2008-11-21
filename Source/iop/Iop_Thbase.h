#ifndef _IOP_THBASE_H_
#define _IOP_THBASE_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
    class CThbase : public CModule
    {
    public:
                        CThbase(CIopBios&, uint8*);
        virtual         ~CThbase();

        std::string     GetId() const;
        void            Invoke(CMIPS&, unsigned int);

    private:
        struct THREAD
        {
            uint32 attributes;
            uint32 options;
            uint32 threadProc;
            uint32 stackSize;
            uint32 priority;
        };

        uint32          CreateThread(const THREAD*);
        uint32          StartThread(uint32, uint32);
        uint32          DelayThread(uint32);
        uint32          GetThreadId();
        uint32          SleepThread();
        uint32          WakeupThread(uint32);
		uint32			iWakeupThread(uint32);
		uint32			GetSystemTime(uint32);
		void			USecToSysClock(uint32, uint32);
		void			SysClockToUSec(uint32, uint32, uint32);

        uint8*          m_ram;
        CIopBios&       m_bios;
    };
}

#endif
