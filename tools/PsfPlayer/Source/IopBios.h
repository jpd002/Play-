#ifndef _IOPBIOS_H_
#define _IOPBIOS_H_

#include "MIPSAssembler.h"
#include "MIPS.h"
#include "ELF.h"
#include "Iop_Ioman.h"
#include "Iop_Stdio.h"
#include "Iop_Sysmem.h"
#include "Iop_Modload.h"

class CIopBios
{
public:
                            CIopBios(uint32, CMIPS&, uint8*, uint32);
    virtual                 ~CIopBios();

    void                    LoadAndStartModule(const char*, const char*, unsigned int);
    void                    SysCallHandler();

    Iop::CIoman*            GetIoman();
    void                    RegisterModule(Iop::CModule*);

private:
    enum DEFAULT_STACKSIZE
    {
        DEFAULT_STACKSIZE = 0x8000,
    };

    struct THREADCONTEXT
    {
        uint32      gpr[0x20];
        uint32      epc;
        uint32      delayJump;
    };

    struct THREAD
    {
        uint32          id;
        uint32          priority;
        THREADCONTEXT   context;
    };

    typedef std::multimap<uint32, THREAD> ThreadMapType;
    typedef std::map<std::string, Iop::CModule*> IopModuleMapType;

    THREAD&                 GetThread(uint32);
    ThreadMapType::iterator GetThreadPosition(uint32);
    uint32                  CreateThread(uint32);
    void                    ExitCurrentThread();
    void                    LoadThreadContext(uint32);
    void                    SaveThreadContext(uint32);
    void                    Reschedule();

    uint32                  LoadExecutable(const char*);
    void                    RelocateElf(CELF&, uint32);
    std::string             ReadModuleName(uint32);
    uint32                  Push(uint32&, const uint8*, uint32);

    uint32                  AssembleThreadFinish(CMIPSAssembler&);

    CMIPS&                  m_cpu;
    uint8*                  m_ram;
    uint32                  m_baseAddress;
    uint32                  m_threadFinishAddress;
    uint32                  m_nextThreadId;
    uint32                  m_currentThreadId;
    ThreadMapType           m_threads;
    IopModuleMapType        m_modules;
    Iop::CStdio*            m_stdio;
    Iop::CIoman*            m_ioman;
    Iop::CSysmem*           m_sysmem;
    Iop::CModload*          m_modload;
};

#endif
