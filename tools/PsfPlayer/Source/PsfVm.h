#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "MIPS.h"
#include "PsfFs.h"
#include "VirtualMachine.h"
#include <string>
#include <boost/thread.hpp>
#include "IopBios.h"

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

    static unsigned int     TickFunctionStub(unsigned int, CMIPS*);
    unsigned int            TickFunction(unsigned int);
    static void             SysCallHandlerStub(CMIPS*);

    void                    EmulationProc();

    CMIPS                   m_cpu;
    CPsfFs                  m_fileSystem;
    CIopBios*               m_bios;
    uint8*                  m_ram;
    STATUS                  m_status;
    bool                    m_pauseAck;
    boost::thread*          m_emuThread;
    bool                    m_singleStep;
};

#endif
