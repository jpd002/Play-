#ifndef _TIMER_H_
#define _TIMER_H_

#include "Types.h"
#include "INTC.h"

class CTimer
{
public:
    enum REGISTERS
    {
        T0_COUNT	= 0x10000000,
        T0_MODE		= 0x10000010,
        T0_COMP		= 0x10000020,
        T0_HOLD		= 0x10000030,
    };

                            CTimer(CINTC&);
    virtual                 ~CTimer();

    void                    Reset();

    void                    Count(unsigned int);

    uint32                  GetRegister(uint32);
    void                    SetRegister(uint32, uint32);


private:
    void                    DisassembleGet(uint32);
    void                    DisassembleSet(uint32, uint32);

    struct TIMER
    {
        uint32	nCOUNT;
        uint32	nMODE;
        uint32	nCOMP;
        uint32	nHOLD;
    };

    TIMER                   m_Timer[4];
    CINTC&                  m_intc;
};

#endif
