#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "PtrStream.h"
#include "Iop_Intc.h"
#include "lexical_cast_ex.h"
#include "xml/FilteringNodeIterator.h"

#ifdef _IOP_EMULATE_MODULES
#include "Iop_DbcMan320.h"
#include "Iop_LibSd.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_McServ.h"
#include "Iop_FileIo.h"
#include "Iop_Unknown.h"
#include "Iop_Unknown2.h"
#endif

#include "Iop_SifManNull.h"
#include "Iop_SifCmd.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thevent.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include "Iop_Vblank.h"
#include <vector>

#define LOGNAME "iop_bios"

using namespace std;
using namespace Framework;

CIopBios::CIopBios(uint32 baseAddress, uint32 clockFrequency, CMIPS& cpu, uint8* ram, uint32 ramSize) :
m_baseAddress(baseAddress),
m_cpu(cpu),
m_ram(ram),
m_ramSize(ramSize),
m_nextThreadId(1),
m_nextSemaphoreId(1),
m_sifMan(NULL),
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
m_threadFinishAddress(0),
m_clockFrequency(clockFrequency),
m_currentTime(0)
{

}

CIopBios::~CIopBios()
{
    DeleteModules();
}

void CIopBios::Reset(Iop::CSifMan* sifMan)
{
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[m_baseAddress]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
	}

	//0xBE00000 = Stupid constant to make FFX PSF happy
	m_currentTime = 0xBE00000;

	m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_INT;

    m_intrHandlers.clear();
    m_moduleTags.clear();

    DeleteModules();

    if(sifMan == NULL)
    {
        m_sifMan = new Iop::CSifManNull();
    }
    else
    {
        m_sifMan = sifMan;
    }

    //Register built-in modules
    {
        m_stdio = new Iop::CStdio(m_ram);
        RegisterModule(m_stdio);
    }
    {
        m_ioman = new Iop::CIoman(m_ram);
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
    {
        RegisterModule(new Iop::CVblank(*this));
    }
	{
		RegisterModule(m_sifMan);
	}
    {
        RegisterModule(new Iop::CSifCmd(*this, *m_sifMan, *m_sysmem, m_ram));
    }
#ifdef _IOP_EMULATE_MODULES
    {
        RegisterModule(new Iop::CFileIo(*m_sifMan, *m_ioman));
    }
    {
        m_cdvdfsv = new Iop::CCdvdfsv(*m_sifMan);
        RegisterModule(m_cdvdfsv);
    }
    {
        RegisterModule(new Iop::CLibSd(*m_sifMan));
    }
    {
        RegisterModule(new Iop::CMcServ(*m_sifMan));
    }
    {
        m_dbcman = new Iop::CDbcMan(*m_sifMan);
        RegisterModule(m_dbcman);
    }
    {
        RegisterModule(new Iop::CDbcMan320(*m_sifMan, *m_dbcman));
    }
    {
        m_padman = new Iop::CPadMan(*m_sifMan);
        RegisterModule(m_padman);
    }

    //Custom modules
    {
        RegisterModule(new Iop::CUnknown(*m_sifMan));
    }
    {
        RegisterModule(new Iop::CUnknown2(*m_sifMan));
    }
#endif

    const int sifDmaBufferSize = 0x1000;
    uint32 sifDmaBufferPtr = m_sysmem->AllocateMemory(sifDmaBufferSize, 0);
#ifndef _NULL_SIFMAN
    m_sifMan->SetDmaBuffer(m_ram + sifDmaBufferPtr, sifDmaBufferSize);
#endif

    Reschedule();
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
    CELF module(m_ram + modulePtr);
    LoadAndStartModule(module, "", args, argsLength);
}

void CIopBios::LoadAndStartModule(CELF& elf, const char* path, const char* args, unsigned int argsLength)
{
    ExecutableRange moduleRange;
    uint32 entryPoint = LoadExecutable(elf, moduleRange);

    ModuleListIterator moduleIterator(FindModule(moduleRange.first, moduleRange.second));
    if(moduleIterator == m_moduleTags.end())
    {
        MIPSMODULE module;
        module.name  = GetModuleNameFromPath(path);
        module.begin = moduleRange.first;
        module.end   = moduleRange.second;
        m_moduleTags.push_back(module);
    }

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
	thread.context.gpr[CMIPS::SP] -= 4;

    StartThread(threadId);
    if(m_currentThreadId == -1)
    {
        Reschedule();
    }

#ifdef _DEBUG
    m_cpu.m_pAnalysis->Analyse(moduleRange.first, moduleRange.second);

	bool functionAdded = false;
	//Look for import tables
	for(uint32 address = moduleRange.first; address < moduleRange.second; address += 4)
	{
		if(m_cpu.m_pMemoryMap->GetWord(address) == 0x41E00000)
		{
			if(m_cpu.m_pMemoryMap->GetWord(address + 4) != 0) continue;
			
			uint32 version = m_cpu.m_pMemoryMap->GetWord(address + 8);
			string moduleName = ReadModuleName(address + 0xC);
	        IopModuleMapType::iterator module(m_modules.find(moduleName));

			size_t moduleNameLength = moduleName.length();
			uint32 entryAddress = address + 0x0C + ((moduleNameLength + 3) & ~0x03);
			while(m_cpu.m_pMemoryMap->GetWord(entryAddress) == 0x03E00008)
			{
				uint32 target = m_cpu.m_pMemoryMap->GetWord(entryAddress + 4);
				uint32 functionId = target & 0xFFFF;
                string functionName = "unknown";
                if(module != m_modules.end())
                {
				    functionName = (module->second)->GetFunctionName(functionId);
                }
				if(m_cpu.m_Functions.Find(address) == NULL)
				{
					m_cpu.m_Functions.InsertTag(entryAddress, (string(moduleName) + "_" + functionName).c_str());
					functionAdded = true;
				}
				entryAddress += 8;
			}
		}
	}

	if(functionAdded)
	{
		m_cpu.m_Functions.m_OnTagListChanged();
	}

    CLog::GetInstance().Print(LOGNAME, "Loaded IOP module '%s' @ 0x%0.8X.\r\n", 
        path, moduleRange.first);
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
    thread.nextActivateTime = GetCurrentTime() + MicroSecToClock(delay);
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

uint32 CIopBios::WakeupThread(uint32 threadId, bool inInterrupt)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: WakeupThread(threadId = %i);\r\n", 
        m_currentThreadId, threadId);
#endif

    THREAD& thread = GetThread(threadId);
    if(thread.status == THREAD_STATUS_SLEEPING)
    {
        thread.status = THREAD_STATUS_RUNNING;
		if(!inInterrupt)
		{
			m_rescheduleNeeded = true;
		}
    }
    else
    {
        thread.wakeupCount++;
    }
    return thread.wakeupCount;
}

uint32 CIopBios::GetCurrentThreadId()
{
    return m_currentThreadId;
}

void CIopBios::ExitCurrentThread()
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%d : ExitCurrentThread();\r\n", m_currentThreadId);
#endif
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
//    if(nextThreadId == -1)
//    {
//        //Try without checking activation time
//        nextThreadId = GetNextReadyThread(false);
//    }

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
#ifdef _DEBUG
	if(nextThreadId != m_currentThreadId)
	{
		CLog::GetInstance().Print(LOGNAME, "Switched over to thread %i.\r\n", nextThreadId);
	}
#endif
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
	return m_currentTime;
}

uint64 CIopBios::MilliSecToClock(uint32 value)
{
	return (static_cast<uint64>(value) * static_cast<uint64>(m_clockFrequency)) / 1000;
}

uint64 CIopBios::MicroSecToClock(uint32 value)
{
	return (static_cast<uint64>(value) * static_cast<uint64>(m_clockFrequency)) / 1000000;
}

uint64 CIopBios::ClockToMicroSec(uint64 clock)
{
	return (clock * 1000000) / m_clockFrequency;
}

void CIopBios::CountTicks(uint32 ticks)
{
	m_currentTime += ticks;
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

Iop::CCdvdfsv* CIopBios::GetCdvdfsv()
{
    return m_cdvdfsv;
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
    m_rescheduleNeeded = false;

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

CIopBios::ModuleListIterator CIopBios::FindModule(uint32 beginAddress, uint32 endAddress)
{
    for(ModuleListIterator moduleIterator(m_moduleTags.begin());
        m_moduleTags.end() != moduleIterator; moduleIterator++)
    {
        MIPSMODULE& module(*moduleIterator);
        if(beginAddress == module.begin && endAddress == module.end)
        {
            return moduleIterator;
        }
    }
    return m_moduleTags.end();
}

#ifdef DEBUGGER_INCLUDED

#define TAGS_SECTION_IOP_MODULES                        ("modules")
#define TAGS_SECTION_IOP_MODULES_MODULE                 ("module")
#define TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS    ("beginAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS      ("endAddress")
#define TAGS_SECTION_IOP_MODULES_MODULE_NAME            ("name")

void CIopBios::LoadDebugTags(Xml::CNode* root)
{
    Xml::CNode* moduleSection = root->Select(TAGS_SECTION_IOP_MODULES);
    if(moduleSection == NULL) return;

    for(Xml::CFilteringNodeIterator nodeIterator(moduleSection, TAGS_SECTION_IOP_MODULES_MODULE);
        !nodeIterator.IsEnd(); nodeIterator++)
    {
        Xml::CNode* moduleNode(*nodeIterator);
        const char* moduleName      = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME);
        const char* beginAddress    = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS);
        const char* endAddress      = moduleNode->GetAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS);
        if(!moduleName || !beginAddress || !endAddress) continue;
        MIPSMODULE module;
        module.name     = moduleName;
        module.begin    = lexical_cast_hex<string>(beginAddress);
        module.end      = lexical_cast_hex<string>(endAddress);
        m_moduleTags.push_back(module);
    }
}

void CIopBios::SaveDebugTags(Xml::CNode* root)
{
    Xml::CNode* moduleSection = new Xml::CNode(TAGS_SECTION_IOP_MODULES, true);

    for(MipsModuleList::const_iterator moduleIterator(m_moduleTags.begin());
        m_moduleTags.end() != moduleIterator; moduleIterator++)
    {
        const MIPSMODULE& module(*moduleIterator);
        Xml::CNode* moduleNode = new Xml::CNode(TAGS_SECTION_IOP_MODULES_MODULE, true);
        moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS,   lexical_cast_hex<string>(module.begin, 8).c_str());
        moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS,     lexical_cast_hex<string>(module.end, 8).c_str());
        moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_NAME,           module.name.c_str());
        moduleSection->InsertNode(moduleNode);
    }

    root->InsertNode(moduleSection);
}

MipsModuleList CIopBios::GetModuleList()
{
	return m_moduleTags;
}

#endif

//void CIopBios::LoadModuleTags(const MODULETAG& module, CMIPSTags& tags, const char* tagCollectionName)
//{
//    CMIPSTags moduleTags;
//    moduleTags.Unserialize((module.name + "." + string(tagCollectionName)).c_str());
//    for(CMIPSTags::TagIterator tag(moduleTags.GetTagsBegin());
//        tag != moduleTags.GetTagsEnd(); tag++)
//    {
//        tags.InsertTag(tag->first + module.begin, tag->second.c_str());
//    }
//    tags.m_OnTagListChanged();
//}
//
//void CIopBios::SaveAllModulesTags(CMIPSTags& tags, const char* tagCollectionName)
//{
//    for(LoadedModuleListType::const_iterator moduleIterator(m_moduleTags.begin());
//        m_moduleTags.end() != moduleIterator; moduleIterator++)
//    {
//        const MODULETAG& module(*moduleIterator);
//        CMIPSTags moduleTags;
//        for(CMIPSTags::TagIterator tag(tags.GetTagsBegin());
//            tag != tags.GetTagsEnd(); tag++)
//        {
//            uint32 tagAddress = tag->first;
//            if(tagAddress >= module.begin && tagAddress <= module.end)
//            {
//                moduleTags.InsertTag(tagAddress - module.begin, tag->second.c_str());
//            }
//        }
//        moduleTags.Serialize((module.name + "." + string(tagCollectionName)).c_str());
//    }
//}

void CIopBios::DeleteModules()
{
    while(m_modules.size() != 0)
    {
        delete m_modules.begin()->second;
        m_modules.erase(m_modules.begin());
    }

    m_sifMan = NULL;
    m_stdio = NULL;
    m_ioman = NULL;
    m_sysmem = NULL;
    m_modload = NULL;
#ifdef _IOP_EMULATE_MODULES
    m_dbcman = NULL;
    m_padman = NULL;
    m_cdvdfsv = NULL;
#endif
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

    //Clean BSS sections
    {
        const ELFHEADER& header(elf.GetHeader());
        for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
        {
            ELFSECTIONHEADER* sectionHeader = elf.GetSection(i);
            if(sectionHeader->nType == CELF::SHT_NOBITS && sectionHeader->nStart != 0)
            {
                memset(m_ram + baseAddress + sectionHeader->nStart, 0, sectionHeader->nSize);
            }
        }
    }

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
