#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "MIPS.h"
#include "PsfFs.h"
#include "VirtualMachine.h"
#include <string>
#include <boost/thread.hpp>
#include "Iop_Stdio.h"
#include "Iop_Ioman.h"
#include "Iop_Sysmem.h"
#include "Ioman_Psf.h"

class CPsfVm : public CVirtualMachine
{
public:
                            CPsfVm(const char*);
    virtual                 ~CPsfVm();

    CMIPS&                  GetCpu();
    STATUS                  GetStatus() const;
    void                    Pause();
    void                    Resume();

private:
    typedef std::map<std::string, Iop::CModule*> IopModuleMapType;

    enum RAMSIZE
    {
        RAMSIZE = 0x00400000,
    };

    enum DEFAULT_STACKSIZE
    {
        DEFAULT_STACKSIZE = 0x8000,
    };

    uint32                  LoadIopModule(const char*, uint32);
    uint32                  Push(uint32&, const uint8*, uint32);
    static unsigned int     TickFunctionStub(unsigned int, CMIPS*);
    static void             SysCallHandlerStub(CMIPS*);
    unsigned int            TickFunction(unsigned int);
    void                    SysCallHandler();

    std::string             ReadModuleName(uint32);
    void                    RegisterModule(Iop::CModule*);

    void                    EmulationProc();

    CMIPS                   m_cpu;
    CPsfFs                  m_fileSystem;
    uint8*                  m_ram;
    STATUS                  m_status;
    bool                    m_pauseAck;
    boost::thread*          m_emuThread;

    IopModuleMapType        m_iopModules;
    Iop::CStdio*            m_iopStdio;
    Iop::CIoman*            m_iopIoman;
    Iop::CSysmem*           m_iopSysmem;
};

#endif
