#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "MIPS.h"
#include "PsfFs.h"
#include "Spu2.h"
#include "VirtualMachine.h"
#include <string>
#include <boost/thread.hpp>
#include "IopBios.h"
#include "Iop_Intc.h"

class CPsfVm : public CVirtualMachine
{
public:
                            CPsfVm(const char*);
    virtual                 ~CPsfVm();

    CMIPS&                  GetCpu();
    STATUS                  GetStatus() const;
    void                    Step();
    void                    Pause();
    void                    Resume();

private:
    enum RAMSIZE
    {
        RAMSIZE = 0x00400000,
    };

    enum SPUADDRESS
    {
        SPUBEGIN = 0x1F800000,
        SPUEND = 0x1F900800,
    };

    static unsigned int     TickFunctionStub(unsigned int, CMIPS*);
    unsigned int            TickFunction(unsigned int);
    static void             SysCallHandlerStub(CMIPS*);

    void                    ProcessTimer();
    void                    CheckInterrupts();
    void                    EmulationProc();

    CMIPS                   m_cpu;
    CPsfFs                  m_fileSystem;
    CIopBios*               m_bios;
    CSpu2                   m_spu;
    uint8*                  m_ram;
    Iop::CIntc              m_intc;
    STATUS                  m_status;
    bool                    m_pauseAck;
    boost::thread*          m_emuThread;
    bool                    m_singleStep;
};

#endif
