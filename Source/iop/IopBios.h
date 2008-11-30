#ifndef _IOPBIOS_H_
#define _IOPBIOS_H_

#include <list>
#include "../MIPSAssembler.h"
#include "../MIPS.h"
#include "../ELF.h"
#include "Iop_BiosBase.h"
#include "Iop_SifMan.h"
#include "Iop_Ioman.h"
#include "Iop_Stdio.h"
#include "Iop_Sysmem.h"
#include "Iop_Modload.h"
#ifdef _IOP_EMULATE_MODULES
#include "Iop_DbcMan.h"
#include "Iop_PadMan.h"
#include "Iop_Cdvdfsv.h"
#endif

class CIopBios : public Iop::CBiosBase
{
public:
    struct MODULETAG
    {
        std::string     name;
        uint32          begin;
        uint32          end;
    };

    typedef std::list<MODULETAG> ModuleTagListType;
    typedef ModuleTagListType::iterator ModuleTagIterator;

                            CIopBios(uint32, uint32, CMIPS&, uint8*, uint32);
    virtual                 ~CIopBios();

    void                    LoadAndStartModule(const char*, const char*, unsigned int);
    void                    LoadAndStartModule(uint32, const char*, unsigned int);
    void                    HandleException();
    void                    HandleInterrupt();

	void					CountTicks(uint32);
    uint64                  GetCurrentTime();
	uint64					MilliSecToClock(uint32);
	uint64					MicroSecToClock(uint32);
	uint64					ClockToMicroSec(uint64);

    void                    Reset(Iop::CSifMan*);
#ifdef DEBUGGER_INCLUDED
    void                    LoadDebugTags(Framework::Xml::CNode*);
    void                    SaveDebugTags(Framework::Xml::CNode*);
#endif

    ModuleTagIterator       GetModuleTagsBegin();
    ModuleTagIterator       GetModuleTagsEnd();

    Iop::CIoman*            GetIoman();
#ifdef _IOP_EMULATE_MODULES
    Iop::CDbcMan*           GetDbcman();
    Iop::CPadMan*           GetPadman();
    Iop::CCdvdfsv*          GetCdvdfsv();
#endif
    void                    RegisterModule(Iop::CModule*);

    uint32                  CreateThread(uint32, uint32);
    void                    StartThread(uint32, uint32* = NULL);
    void                    DelayThread(uint32);
    uint32                  GetCurrentThreadId();
    void                    SleepThread();
    uint32                  WakeupThread(uint32, bool);

    uint32                  CreateSemaphore(uint32, uint32);
    uint32                  SignalSemaphore(uint32, bool);
    uint32                  WaitSemaphore(uint32);

    bool                    RegisterIntrHandler(uint32, uint32, uint32, uint32);
    bool                    ReleaseIntrHandler(uint32);

private:
    enum DEFAULT_STACKSIZE
    {
        DEFAULT_STACKSIZE = 0x8000,
    };

    enum DEFAULT_PRIORITY
    {
        DEFAULT_PRIORITY = 7,
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
        uint32          status;
        uint32          waitSemaphore;
        uint32          wakeupCount;
        uint64          nextActivateTime;
    };

    struct SEMAPHORE
    {
        uint32          id;
        uint32          count;
        uint32          maxCount;
        uint32          waitCount;
    };

    struct INTRHANDLER
    {
        uint32          line;
        uint32          mode;
        uint32          handler;
        uint32          arg;
    };

    enum THREAD_STATUS
    {
        THREAD_STATUS_CREATED = 1,
        THREAD_STATUS_RUNNING = 2,
        THREAD_STATUS_SLEEPING = 3,
        THREAD_STATUS_ZOMBIE = 4,
        THREAD_STATUS_WAITING = 5,
    };

    typedef std::multimap<uint32, THREAD, std::greater<uint32> > ThreadMapType;
    typedef std::map<std::string, Iop::CModule*> IopModuleMapType;
    typedef std::map<uint32, SEMAPHORE> SemaphoreMapType;
    typedef std::map<uint32, INTRHANDLER> IntrHandlerMapType;
    typedef std::pair<uint32, uint32> ExecutableRange;

    THREAD&                 GetThread(uint32);
    ThreadMapType::iterator GetThreadPosition(uint32);
    void                    ExitCurrentThread();
    void                    LoadThreadContext(uint32);
    void                    SaveThreadContext(uint32);
    void                    Reschedule();
    uint32                  GetNextReadyThread(bool);
	void					ReturnFromException();

    SEMAPHORE&              GetSemaphore(uint32);

    void                    LoadAndStartModule(CELF&, const char*, const char*, unsigned int);
    uint32                  LoadExecutable(CELF&, ExecutableRange&);
    unsigned int            GetElfProgramToLoad(CELF&);
    void                    RelocateElf(CELF&, uint32);
    std::string             ReadModuleName(uint32);
    std::string             GetModuleNameFromPath(const std::string&);
    ModuleTagIterator       FindModule(uint32, uint32);
//    void                    LoadModuleTags(const LOADEDMODULE&, CMIPSTags&, const char*);
//    void                    SaveAllModulesTags(CMIPSTags&, const char*);
#ifdef DEBUGGER_INCLUDED
    void                    LoadLoadedModules(Framework::Xml::CNode*);
    void                    SaveLoadedModules(Framework::Xml::CNode*);
#endif
    void                    DeleteModules();
    uint32                  Push(uint32&, const uint8*, uint32);

    uint32                  AssembleThreadFinish(CMIPSAssembler&);
	uint32					AssembleReturnFromException(CMIPSAssembler&);
	uint32					AssembleIdleFunction(CMIPSAssembler&);

    CMIPS&                  m_cpu;
    uint8*                  m_ram;
    uint32                  m_ramSize;
    uint32                  m_baseAddress;
    uint32                  m_threadFinishAddress;
	uint32					m_returnFromExceptionAddress;
	uint32					m_idleFunctionAddress;
    uint32                  m_nextThreadId;
    uint32                  m_nextSemaphoreId;
    uint32                  m_currentThreadId;
	uint32					m_clockFrequency;
	uint64					m_currentTime;
    bool                    m_rescheduleNeeded;
    ThreadMapType           m_threads;
    SemaphoreMapType        m_semaphores;
    IntrHandlerMapType      m_intrHandlers;
    IopModuleMapType        m_modules;
    ModuleTagListType       m_moduleTags;
    Iop::CSifMan*           m_sifMan;
    Iop::CStdio*            m_stdio;
    Iop::CIoman*            m_ioman;
    Iop::CSysmem*           m_sysmem;
    Iop::CModload*          m_modload;
#ifdef _IOP_EMULATE_MODULES
    Iop::CDbcMan*           m_dbcman;
    Iop::CPadMan*           m_padman;
    Iop::CCdvdfsv*          m_cdvdfsv;
#endif
};

#endif
