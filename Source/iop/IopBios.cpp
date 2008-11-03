#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "PtrStream.h"
#include "Iop_Intc.h"

#ifdef _IOP_EMULATE_MODULES
#include "Iop_DbcMan320.h"
#include "Iop_LibSd.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_McServ.h"
#include "Iop_Unknown.h"
#include "Iop_Unknown2.h"
#endif

#include "Iop_SifManNull.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thevent.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include <vector>
#include <time.h>

#define LOGNAME "iop_bios"

using namespace std;
using namespace Framework;

CIopBios::CIopBios(uint32 baseAddress, CMIPS& cpu, uint8* ram, uint32 ramSize, CSIF& sif, CISO9660*& iso) :
m_baseAddress(baseAddress),
m_cpu(cpu),
m_ram(ram),
m_ramSize(ramSize),
m_sif(sif),
m_sifMan(NULL),
m_iso(iso),
m_nextThreadId(1),
m_nextSemaphoreId(1),
m_stdio(NULL),
m_sysmem(NULL),
m_ioman(NULL),
m_modload(NULL),
#ifdef _IOP_EMULATE_MODULES
m_dbcman(NULL),
m_padman(NULL),
#endif
m_rescheduleNeeded(false),
m_currentThreadId(-1),
m_threadFinishAddress(0)
{
#ifdef _NULL_SIFMAN
    m_sifMan = new Iop::CSifManNull();
#else
    int dasddasd +dak;l;
#endif
}

CIopBios::~CIopBios()
{
#ifdef _DEBUG
    SaveAllModulesTags(m_cpu.m_Comments, "comments");
    SaveAllModulesTags(m_cpu.m_Functions, "functions");
#endif
    DeleteModules();
}

void CIopBios::Reset()
{
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[m_baseAddress]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
	}

	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_INT;

    m_intrHandlers.clear();

    DeleteModules();

    //Register built-in modules
    {
        m_stdio = new Iop::CStdio(m_ram);
        RegisterModule(m_stdio);
    }
    {
        m_ioman = new Iop::CIoman(m_ram, *m_sifMan);
        RegisterModule(m_ioman);
    }
    {
        m_sysmem = new Iop::CSysmem(0x1000, m_ramSize, *m_stdio, *m_sifMan);
        RegisterModule(m_sysmem);
    }
    {
        m_modload = new Iop::CModload(*this, m_ram);
        RegisterModule(m_modload);
    }
    {
        RegisterModule(new Iop::CSysclib(m_ram, *m_stdio));
    }
    {
        RegisterModule(new Iop::CLoadcore(*this, m_ram, *m_sifMan));
    }
    {
        RegisterModule(new Iop::CThbase(*this, m_ram));
    }
    {
        RegisterModule(new Iop::CThsema(*this, m_ram));
    }
    {
        RegisterModule(new Iop::CThevent(*this, m_ram));
    }
    {
        RegisterModule(new Iop::CTimrman(*this));
    }
    {
        RegisterModule(new Iop::CIntrman(*this, m_ram));
    }
#ifdef _IOP_EMULATE_MODULES
    {
        RegisterModule(new Iop::CCdvdfsv(m_iso, m_sif));
    }
    {
        RegisterModule(new Iop::CLibSd(m_sif));
    }
    {
        RegisterModule(new Iop::CMcServ(m_sif));
    }
    {
        m_dbcman = new Iop::CDbcMan(m_sif);
        RegisterModule(m_dbcman);
    }
    {
        RegisterModule(new Iop::CDbcMan320(m_sif, *m_dbcman));
    }
    {
        m_padman = new Iop::CPadMan(m_sif);
        RegisterModule(m_padman);
    }

    //Custom modules
    {
        RegisterModule(new Iop::CUnknown(m_sif));
    }
    {
        RegisterModule(new Iop::CUnknown2(m_sif));
    }
#endif

    const int sifDmaBufferSize = 0x1000;
    uint32 sifDmaBufferPtr = m_sysmem->AllocateMemory(sifDmaBufferSize, 0);
#ifndef _NULL_SIFMAN
    m_sif.SetDmaBuffer(m_ram + sifDmaBufferPtr, sifDmaBufferSize);
#endif
}

void CIopBios::LoadAndStartModule(const char* path, const char* args, unsigned int argsLength)
{
    uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::O_RDONLY, path);
    if(handle & 0x80000000)
    {
        throw runtime_error("Couldn't open executable for reading.");
    }
    Iop::CIoman::CFile file(handle, *m_ioman);
    CStream* stream = m_ioman->GetFileStream(file);
    CELF* module = new CElfFile(*stream);
    LoadAndStartModule(*module, path, args, argsLength);
}

void CIopBios::LoadAndStartModule(uint32 modulePtr, const char* args, unsigned int argsLength)
{
//    CELF module(m_ram + modulePtr);
//    LoadAndStartModule(module, "", args, argsLength);
}

void CIopBios::LoadAndStartModule(CELF& elf, const char* path, const char* args, unsigned int argsLength)
{
    ExecutableRange moduleRange;
    uint32 entryPoint = LoadExecutable(elf, moduleRange);

    LOADEDMODULE loadedModule;
    loadedModule.name = GetModuleNameFromPath(path);
    loadedModule.begin = moduleRange.first;
    loadedModule.end = moduleRange.second;
    m_loadedModules.push_back(loadedModule);

    uint32 threadId = CreateThread(entryPoint, DEFAULT_PRIORITY);
    THREAD& thread = GetThread(threadId);

    typedef vector<uint32> ParamListType;
    ParamListType paramList;

    paramList.push_back(Push(
        thread.context.gpr[CMIPS::SP],
        reinterpret_cast<const uint8*>(path),
        static_cast<uint32>(strlen(path)) + 1));
    if(argsLength != 0 && args != NULL)
    {
        unsigned int argsPos = 0;
        while(argsPos < argsLength)
        {
            const char* arg = args + argsPos;
            unsigned int argLength = strlen(arg) + 1;
            argsPos += argLength;
            uint32 argAddress = Push(
                thread.context.gpr[CMIPS::SP],
                reinterpret_cast<const uint8*>(arg),
                static_cast<uint32>(argLength));
            paramList.push_back(argAddress);
        }
    }
    thread.context.gpr[CMIPS::A0] = static_cast<uint32>(paramList.size());
    for(ParamListType::reverse_iterator param(paramList.rbegin());
        paramList.rend() != param; param++)
    {
        thread.context.gpr[CMIPS::A1] = Push(
            thread.context.gpr[CMIPS::SP],
            reinterpret_cast<const uint8*>(&(*param)),
            4);
    }

    StartThread(threadId);
    if(m_currentThreadId == -1)
    {
        Reschedule();
    }

#ifdef _DEBUG
    if(loadedModule.name.length())
    {
        LoadModuleTags(loadedModule, m_cpu.m_Comments, "comments");
        LoadModuleTags(loadedModule, m_cpu.m_Functions, "functions");
    }
    m_cpu.m_pAnalysis->Analyse(moduleRange.first, moduleRange.second);
#endif

//	for(int i = 0; i < m_ramSize / 4; i++)
//	{
//		uint32 nVal = ((uint32*)m_ram)[i];
////        if(nVal == 0x34C00)
////        {
////			printf("Allo: 0x%0.8X\r\n", i * 4);
////        }
///*
//        if((nVal & 0xFFFF) == 0xB730)
//        {
//            char mnemonic[256];
//            m_cpu.m_pArch->GetInstructionMnemonic(&m_cpu, i * 4, nVal, mnemonic, 256);
//            printf("Allo: %s, 0x%0.8X\r\n", mnemonic, i * 4);
//        }
//*/
///*
//        if(nVal == 0x2F9B50)
//		{
//			printf("Allo: 0x%0.8X\r\n", i * 4);
//		}
//*/
//
//        if((nVal & 0xFC000000) == 0x0C000000)
//		{
//			nVal &= 0x3FFFFFF;
//			nVal *= 4;
//			if(nVal == 0x41410)
//			{
//				printf("Allo: 0x%0.8X\r\n", i * 4);
//			}
//		}
//
//	}

//    *reinterpret_cast<uint32*>(&m_ram[0x41674]) = 0;
}

CIopBios::THREAD& CIopBios::GetThread(uint32 threadId)
{
    return GetThreadPosition(threadId)->second;
}

CIopBios::ThreadMapType::iterator CIopBios::GetThreadPosition(uint32 threadId)
{
    for(ThreadMapType::iterator thread(m_threads.begin());
        thread != m_threads.end(); thread++)
    {
        if(thread->second.id == threadId)
        {
            return thread;
        }
    }
    throw runtime_error("Unexisting thread id.");
}

uint32 CIopBios::CreateThread(uint32 threadProc, uint32 priority)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: CreateThread(threadProc = 0x%0.8X, priority = %d);\r\n", 
        m_currentThreadId, threadProc, priority);
#endif

    THREAD thread;
    memset(&thread, 0, sizeof(thread));
    thread.context.delayJump = 1;
    uint32 stackBaseAddress = m_sysmem->AllocateMemory(DEFAULT_STACKSIZE, 0);
    thread.id = m_nextThreadId++;
    thread.priority = priority;
    thread.status = THREAD_STATUS_CREATED;
    thread.context.epc = threadProc;
    thread.nextActivateTime = 0;
    thread.context.gpr[CMIPS::RA] = m_threadFinishAddress;
    thread.context.gpr[CMIPS::SP] = stackBaseAddress;
    m_threads.insert(ThreadMapType::value_type(thread.priority, thread));
    return thread.id;
}

void CIopBios::StartThread(uint32 threadId, uint32* param)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: StartThread(threadId = %i, param = 0x%0.8X);\r\n", 
        m_currentThreadId, threadId, param);
#endif

    THREAD& thread = GetThread(threadId);
    if(thread.status != THREAD_STATUS_CREATED)
    {
        throw runtime_error("Invalid thread state.");
    }
    thread.status = THREAD_STATUS_RUNNING;
    if(param != NULL)
    {
        thread.context.gpr[CMIPS::A0] = *param;
    }
    m_rescheduleNeeded = true;
}

void CIopBios::DelayThread(uint32 delay)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: DelayThread(delay = %i);\r\n", 
        m_currentThreadId, delay);
#endif

    //TODO : Need to delay or something...
    THREAD& thread = GetThread(m_currentThreadId);
    thread.nextActivateTime = GetCurrentTime() + delay;
    m_rescheduleNeeded = true;
}

void CIopBios::SleepThread()
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: SleepThread();\r\n", 
        m_currentThreadId);
#endif

    THREAD& thread = GetThread(m_currentThreadId);
    if(thread.status != THREAD_STATUS_RUNNING)
    {
        throw runtime_error("Thread isn't running.");
    }
    if(thread.wakeupCount == 0)
    {
        thread.status = THREAD_STATUS_SLEEPING;
        m_rescheduleNeeded = true;
    }
    else
    {
        thread.wakeupCount--;
    }
}

uint32 CIopBios::WakeupThread(uint32 threadId)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: WakeupThread(threadId = %i);\r\n", 
        m_currentThreadId, threadId);
#endif

    THREAD& thread = GetThread(threadId);
    if(thread.status == THREAD_STATUS_SLEEPING)
    {
        thread.status = THREAD_STATUS_RUNNING;
        m_rescheduleNeeded = true;
    }
    else
    {
        thread.wakeupCount++;
    }
    return thread.wakeupCount;
}

uint32 CIopBios::GetThreadId()
{
    return m_currentThreadId;
}

void CIopBios::ExitCurrentThread()
{
    ThreadMapType::iterator thread = GetThreadPosition(m_currentThreadId);
    m_threads.erase(thread);
    m_currentThreadId = -1;
    m_rescheduleNeeded = true;
}

void CIopBios::LoadThreadContext(uint32 threadId)
{
    THREAD& thread = GetThread(threadId);
    for(unsigned int i = 0; i < 32; i++)
    {
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
        m_cpu.m_State.nGPR[i].nD0 = static_cast<int32>(thread.context.gpr[i]);
    }
    m_cpu.m_State.nPC = thread.context.epc;
    m_cpu.m_State.nDelayedJumpAddr = thread.context.delayJump;
}

void CIopBios::SaveThreadContext(uint32 threadId)
{
    THREAD& thread = GetThread(threadId);
    for(unsigned int i = 0; i < 32; i++)
    {
		if(i == CMIPS::R0) continue;
		if(i == CMIPS::K0) continue;
		if(i == CMIPS::K1) continue;
        thread.context.gpr[i] = m_cpu.m_State.nGPR[i].nV0;
    }
    thread.context.epc = m_cpu.m_State.nPC;
    thread.context.delayJump = m_cpu.m_State.nDelayedJumpAddr;
}

void CIopBios::Reschedule()
{
    if(m_currentThreadId != -1)
    {
        SaveThreadContext(m_currentThreadId);
        //Reinsert the thread into the map
        ThreadMapType::iterator threadPosition = GetThreadPosition(m_currentThreadId);
        THREAD thread(threadPosition->second);
        m_threads.erase(threadPosition);
        m_threads.insert(ThreadMapType::value_type(thread.priority, thread));
    }

    uint32 nextThreadId = GetNextReadyThread(true);
    if(nextThreadId == -1)
    {
        //Try without checking activation time
        nextThreadId = GetNextReadyThread(false);
    }

    if(nextThreadId == -1)
    {
#ifdef _DEBUG
//		printf("Warning, no thread available for running.\r\n");
#endif
		m_cpu.m_State.nPC = m_idleFunctionAddress;
    }
	else
	{
		LoadThreadContext(nextThreadId);
	}
	m_currentThreadId = nextThreadId;
	m_cpu.m_nQuota = 1;
}

uint32 CIopBios::GetNextReadyThread(bool checkActivateTime)
{
    if(checkActivateTime)
    {
        for(ThreadMapType::const_iterator thread(m_threads.begin()); 
            thread != m_threads.end(); thread++)
        {
            const THREAD& nextThread = thread->second;
            if(GetCurrentTime() <= nextThread.nextActivateTime) continue;
            if(nextThread.status == THREAD_STATUS_RUNNING)
            {
                return nextThread.id;
            }
        }
        return -1;
    }
    else
    {
        //Find the thread with the earliest wakeup time
        uint64 activateTime = -1;
        uint32 nextThreadId = -1;
        for(ThreadMapType::const_iterator thread(m_threads.begin()); 
            thread != m_threads.end(); thread++)
        {
            const THREAD& nextThread = thread->second;
            if(nextThread.status != THREAD_STATUS_RUNNING) continue;
            if(nextThread.nextActivateTime < activateTime)
            {
                nextThreadId = nextThread.id;
            }
        }
        return nextThreadId;
    }
}

uint64 CIopBios::GetCurrentTime()
{
    return (clock() * 1000) / CLOCKS_PER_SEC;
}

CIopBios::SEMAPHORE& CIopBios::GetSemaphore(uint32 semaphoreId)
{
    SemaphoreMapType::iterator semaphore(m_semaphores.find(semaphoreId));
    if(semaphore == m_semaphores.end())
    {
        throw runtime_error("Invalid semaphore id.");
    }
    return semaphore->second;
}

uint32 CIopBios::CreateSemaphore(uint32 initialCount, uint32 maxCount)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: CreateSemaphore(initialCount = %i, maxCount = %i);\r\n", 
        m_currentThreadId, initialCount, maxCount);
#endif

    SEMAPHORE semaphore;
    memset(&semaphore, 0, sizeof(SEMAPHORE));
    semaphore.count = initialCount;
    semaphore.maxCount = maxCount;
    semaphore.id = m_nextSemaphoreId++;
    semaphore.waitCount = 0;
    m_semaphores[semaphore.id] = semaphore;
    return semaphore.id;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId, bool inInterrupt)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: SignalSemaphore(semaphoreId = %i);\r\n", 
        m_currentThreadId, semaphoreId);
#endif

    SEMAPHORE& semaphore = GetSemaphore(semaphoreId);
    if(semaphore.waitCount != 0)
    {
        for(ThreadMapType::iterator threadIterator(m_threads.begin());
            m_threads.end() != threadIterator; ++threadIterator)
        {
            THREAD& thread(threadIterator->second);
            if(thread.waitSemaphore == semaphoreId)
            {
                if(thread.status != THREAD_STATUS_WAITING)
                {
                    throw runtime_error("Thread not waiting (inconsistent state).");
                }
                thread.status = THREAD_STATUS_RUNNING;
                thread.waitSemaphore = 0;
				if(!inInterrupt)
				{
					m_rescheduleNeeded = true;
				}
                semaphore.waitCount--;
                if(semaphore.waitCount == 0)
                {
                    break;
                }
            }
        }
    }
    else
    {
        semaphore.count++;
    }
    return semaphore.count;
}

uint32 CIopBios::WaitSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: WaitSemaphore(semaphoreId = %i);\r\n", 
        m_currentThreadId, semaphoreId);
#endif

    SEMAPHORE& semaphore = GetSemaphore(semaphoreId);
    if(semaphore.count == 0)
    {
        THREAD& thread = GetThread(m_currentThreadId);
        thread.status           = THREAD_STATUS_WAITING;
        thread.waitSemaphore    = semaphoreId;
        semaphore.waitCount++;
        m_rescheduleNeeded      = true;
    }
    else
    {
        semaphore.count--;
    }
    return semaphore.count;
}

Iop::CIoman* CIopBios::GetIoman()
{
    return m_ioman;
}

#ifdef _IOP_EMULATE_MODULES

Iop::CDbcMan* CIopBios::GetDbcman()
{
    return m_dbcman;
}

Iop::CPadMan* CIopBios::GetPadman()
{
    return m_padman;
}

#endif

bool CIopBios::RegisterIntrHandler(uint32 line, uint32 mode, uint32 handler, uint32 arg)
{
    INTRHANDLER intrHandler;
    intrHandler.line = line;
    intrHandler.mode = mode;
    intrHandler.handler = handler;
    intrHandler.arg = arg;
    m_intrHandlers[line] = intrHandler;
    return true;
}

bool CIopBios::ReleaseIntrHandler(uint32 line)
{
    IntrHandlerMapType::iterator handlerIterator(m_intrHandlers.find(line));
    if(handlerIterator == m_intrHandlers.end()) return false;
    m_intrHandlers.erase(handlerIterator);
    return true;
}

uint32 CIopBios::AssembleThreadFinish(CMIPSAssembler& assembler)
{
    uint32 address = m_baseAddress + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0666);
    assembler.SYSCALL();
    return address;
}

uint32 CIopBios::AssembleReturnFromException(CMIPSAssembler& assembler)
{
    uint32 address = m_baseAddress + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0667);
    assembler.SYSCALL();
    return address;
}

uint32 CIopBios::AssembleIdleFunction(CMIPSAssembler& assembler)
{
    uint32 address = m_baseAddress + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0668);
    assembler.SYSCALL();
    return address;
}

void CIopBios::HandleException()
{
    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
    uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
    if(callInstruction == 0x0000000C)
    {
        switch(m_cpu.m_State.nGPR[CMIPS::V0].nV0)
        {
		case 0x666:
            ExitCurrentThread();
			break;
		case 0x667:
			ReturnFromException();
			break;
		case 0x668:
			Reschedule();
			break;
        }
    }
    else
    {
        //Search for the import record
        uint32 instruction = callInstruction;
        while(instruction != 0x41E00000)
        {
            searchAddress -= 4;
            instruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
        }
        uint32 functionId = callInstruction & 0xFFFF;
        uint32 version = m_cpu.m_pMemoryMap->GetWord(searchAddress + 8);
        string moduleName = ReadModuleName(searchAddress + 0x0C);

        IopModuleMapType::iterator module(m_modules.find(moduleName));
        if(module != m_modules.end())
        {
            module->second->Invoke(m_cpu, functionId);
        }
        else
        {
#ifdef _DEBUG
            CLog::GetInstance().Print(LOGNAME, "%0.8X: Trying to call a function from non-existing module (%s, %d).\r\n", 
                m_cpu.m_State.nPC, moduleName.c_str(), functionId);
#endif
        }
    }

    if(m_rescheduleNeeded)
    {
		assert((m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] & CMIPS::STATUS_EXL) == 0);
        m_rescheduleNeeded = false;
        Reschedule();
    }

    m_cpu.m_State.nHasException = 0;
}

void CIopBios::HandleInterrupt()
{
	if(m_cpu.GenerateInterrupt(m_cpu.m_State.nPC))
	{
        //Find first concerned interrupt
        unsigned int line = -1;
        UNION64_32 status(
			m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS0),
			m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::STATUS1));
		UNION64_32 mask(
			m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK0),
			m_cpu.m_pMemoryMap->GetWord(Iop::CIntc::MASK1));
		status.f &= mask.f;
        for(unsigned int i = 0; i < 0x40; i++)
        {
            if(status.f & (1LL << i))
			{
				line = i;
				break;
			}
        }
		assert(line != -1);
		if(line == -1)
		{
			ReturnFromException();
			return;
		}
		status.f = ~(1LL << line);
		m_cpu.m_pMemoryMap->SetWord(Iop::CIntc::STATUS0, status.h0);
		m_cpu.m_pMemoryMap->SetWord(Iop::CIntc::STATUS1, status.h1);
		//Check if there's an handler to call
		{
			IntrHandlerMapType::const_iterator handlerIterator(m_intrHandlers.find(line));
			if(handlerIterator == m_intrHandlers.end())
			{
				ReturnFromException();
				return;
			}
			else
			{
				//Snap out of current thread
				if(m_currentThreadId != -1)
				{
					SaveThreadContext(m_currentThreadId);
				}
				m_currentThreadId = -1;
				m_cpu.m_State.nPC = handlerIterator->second.handler;
				m_cpu.m_State.nGPR[CMIPS::A0].nD0 = static_cast<int32>(handlerIterator->second.arg);
				m_cpu.m_State.nGPR[CMIPS::RA].nD0 = static_cast<int32>(m_returnFromExceptionAddress);
			}
		}
	}
}

void CIopBios::ReturnFromException()
{
	uint32& status = m_cpu.m_State.nCOP0[CCOP_SCU::STATUS];
	assert(status & (CMIPS::STATUS_ERL | CMIPS::STATUS_EXL));
	if(status & CMIPS::STATUS_ERL)
	{
		status &= ~CMIPS::STATUS_ERL;
	}
	else if(status & CMIPS::STATUS_EXL)
	{
		status &= ~CMIPS::STATUS_EXL;
	}
	Reschedule();
}

string CIopBios::GetModuleNameFromPath(const std::string& path)
{
    string::size_type slashPosition;
    slashPosition = path.rfind('/');
    if(slashPosition != string::npos)
    {
        return string(path.begin() + slashPosition + 1, path.end());
    }
    return path;
}

const CIopBios::LOADEDMODULE& CIopBios::GetModuleAtAddress(uint32 address)
{
    for(LoadedModuleListType::const_iterator moduleIterator(m_loadedModules.begin());
        m_loadedModules.end() != moduleIterator; moduleIterator++)
    {
        const LOADEDMODULE& module(*moduleIterator);
        if(address >= module.begin && address <= module.end)
        {
            return module;
        }
    }
    throw runtime_error("No module at specified address.");
}

void CIopBios::LoadModuleTags(const LOADEDMODULE& module, CMIPSTags& tags, const char* tagCollectionName)
{
    CMIPSTags moduleTags;
    moduleTags.Unserialize((module.name + "." + string(tagCollectionName)).c_str());
    for(CMIPSTags::TagIterator tag(moduleTags.GetTagsBegin());
        tag != moduleTags.GetTagsEnd(); tag++)
    {
        tags.InsertTag(tag->first + module.begin, tag->second.c_str());
    }
    tags.m_OnTagListChanged();
}

void CIopBios::SaveAllModulesTags(CMIPSTags& tags, const char* tagCollectionName)
{
    for(LoadedModuleListType::const_iterator moduleIterator(m_loadedModules.begin());
        m_loadedModules.end() != moduleIterator; moduleIterator++)
    {
        const LOADEDMODULE& module(*moduleIterator);
        CMIPSTags moduleTags;
        for(CMIPSTags::TagIterator tag(tags.GetTagsBegin());
            tag != tags.GetTagsEnd(); tag++)
        {
            uint32 tagAddress = tag->first;
            if(tagAddress >= module.begin && tagAddress <= module.end)
            {
                moduleTags.InsertTag(tagAddress - module.begin, tag->second.c_str());
            }
        }
        moduleTags.Serialize((module.name + "." + string(tagCollectionName)).c_str());
    }
}

void CIopBios::DeleteModules()
{
    while(m_modules.size() != 0)
    {
        delete m_modules.begin()->second;
        m_modules.erase(m_modules.begin());
    }
}

string CIopBios::ReadModuleName(uint32 address)
{
    string moduleName;
    while(1)
    {
        uint8 character = m_cpu.m_pMemoryMap->GetByte(address++);
        if(character == 0) break;
        if(character < 0x10) continue;
        moduleName += character;
    }
    return moduleName;
}

void CIopBios::RegisterModule(Iop::CModule* module)
{
    m_modules[module->GetId()] = module;
}

uint32 CIopBios::Push(uint32& address, const uint8* data, uint32 size)
{
    uint32 fixedSize = ((size + 0x3) / 0x4) * 0x4;
    address -= fixedSize;
    memcpy(&m_ram[address], data, size);
    return address;
}

uint32 CIopBios::LoadExecutable(CELF& elf, ExecutableRange& executableRange)
{
    unsigned int programHeaderIndex = GetElfProgramToLoad(elf);
    if(programHeaderIndex == -1)
    {
        throw runtime_error("No program to load.");
    }
    ELFPROGRAMHEADER* programHeader = elf.GetProgram(programHeaderIndex);
//    uint32 baseAddress = m_sysmem->AllocateMemory(elf.m_nLenght, 0);
    uint32 baseAddress = m_sysmem->AllocateMemory(programHeader->nMemorySize, 0);
    RelocateElf(elf, baseAddress);

    memcpy(
        m_ram + baseAddress, 
        elf.GetContent() + programHeader->nOffset, 
        programHeader->nFileSize);

    executableRange.first = baseAddress;
    executableRange.second = baseAddress + programHeader->nMemorySize;
    return baseAddress + elf.GetHeader().nEntryPoint;
}

unsigned int CIopBios::GetElfProgramToLoad(CELF& elf)
{
    unsigned int program = -1;
    const ELFHEADER& header = elf.GetHeader();
    for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
    {
        ELFPROGRAMHEADER* programHeader = elf.GetProgram(i);
        if(programHeader != NULL && programHeader->nType == 1)
        {
            if(program != -1)
            {
                throw runtime_error("Multiple loadable program headers found.");
            }
            program = i;
        }
    }
    return program;
}

void CIopBios::RelocateElf(CELF& elf, uint32 baseAddress)
{
    //Process relocation
    const ELFHEADER& header = elf.GetHeader();
    for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
    {
        ELFSECTIONHEADER* sectionHeader = elf.GetSection(i);
        if(sectionHeader != NULL && sectionHeader->nType == CELF::SHT_REL)
        {
            uint32 lastHi16 = -1;
            uint32 instructionHi16 = -1;
            unsigned int linkedSection = sectionHeader->nInfo;
            unsigned int recordCount = sectionHeader->nSize / 8;
            ELFSECTIONHEADER* relocatedSection = elf.GetSection(linkedSection);
            const uint32* relocationRecord = reinterpret_cast<const uint32*>(elf.GetSectionData(i));
            uint8* relocatedSectionData = reinterpret_cast<uint8*>(const_cast<void*>(elf.GetSectionData(linkedSection)));
            if(relocatedSection == NULL || relocationRecord == NULL || relocatedSection == NULL) continue;
            uint32 sectionBase = relocatedSection->nStart;
            for(unsigned int record = 0; record < recordCount; record++)
            {
                uint32 relocationAddress = relocationRecord[0] - sectionBase;
                uint32 relocationType = relocationRecord[1];
                if(relocationAddress < relocatedSection->nSize) 
                {
                    uint32& instruction = *reinterpret_cast<uint32*>(&relocatedSectionData[relocationAddress]);
                    switch(relocationType)
                    {
                    case CELF::R_MIPS_32:
                        {
                            instruction += baseAddress;
                        }
                        break;
                    case CELF::R_MIPS_26:
                        {
                            uint32 offset = (instruction & 0x03FFFFFF) + (baseAddress >> 2);
                            instruction &= ~0x03FFFFFF;
                            instruction |= offset;
                        }
                        break;
                    case CELF::R_MIPS_HI16:
                        {
                            lastHi16 = relocationAddress;
                            instructionHi16 = instruction;
                        }
                        break;
                    case CELF::R_MIPS_LO16:
                        {
                            if(lastHi16 != -1)
                            {
                                uint32 offset = static_cast<int16>(instruction) + (instructionHi16 << 16);
                                offset += baseAddress;
                                instruction &= ~0xFFFF;
                                instruction |= offset & 0xFFFF;

                                uint32& prevInstruction = *reinterpret_cast<uint32*>(&relocatedSectionData[lastHi16]);
                                prevInstruction &= ~0xFFFF;
                                if(offset & 0x8000) offset += 0x10000;
                                prevInstruction |= offset >> 16;
                                lastHi16 = -1;
                            }
                            else
                            {
#ifdef _DEBUG
                                CLog::GetInstance().Print(LOGNAME, "%s: No HI16 relocation record found for corresponding LO16.\r\n", 
                                    __FUNCTION__);
#endif
                            }
                        }
                        break;
                    default:
                        throw runtime_error("Unknown relocation type.");
                        break;
                    }
                }
                relocationRecord += 2;
            }
        }
    }
}
