#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "../ElfFile.h"
#include "PtrStream.h"
#include "Iop_Intc.h"
#include "lexical_cast_ex.h"
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>
#include <vector>
#include "xml/FilteringNodeIterator.h"
#include "../StructCollectionStateFile.h"

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
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_Thbase.h"
#include "Iop_Thsema.h"
#include "Iop_Thevent.h"
#include "Iop_Timrman.h"
#include "Iop_Intrman.h"
#include "Iop_Vblank.h"

#define LOGNAME "iop_bios"

#define STATE_MODULES                       ("iopbios/dyn_modules.xml")
#define STATE_MODULE_IMPORT_TABLE_ADDRESS   ("ImportTableAddress")

#define BIOS_THREAD_LINK_HEAD_BASE		(CIopBios::CONTROL_BLOCK_START + 0x0000)
#define BIOS_CURRENT_THREAD_ID_BASE     (CIopBios::CONTROL_BLOCK_START + 0x0008)
#define BIOS_CURRENT_TIME_BASE          (CIopBios::CONTROL_BLOCK_START + 0x0010)
#define BIOS_HANDLERS_BASE              (CIopBios::CONTROL_BLOCK_START + 0x0100)
#define BIOS_HANDLERS_END				(BIOS_THREADS_BASE - 1)
#define BIOS_THREADS_BASE				(CIopBios::CONTROL_BLOCK_START + 0x0200)
#define BIOS_THREADS_SIZE				(sizeof(CIopBios::THREAD) * CIopBios::MAX_THREAD)
#define BIOS_SEMAPHORES_BASE			(BIOS_THREADS_BASE + BIOS_THREADS_SIZE)
#define BIOS_SEMAPHORES_SIZE			(sizeof(CIopBios::SEMAPHORE) * CIopBios::MAX_SEMAPHORE)
#define BIOS_INTRHANDLER_BASE			(BIOS_SEMAPHORES_BASE + BIOS_SEMAPHORES_SIZE)
#define BIOS_INTRHANDLER_SIZE			(sizeof(CIopBios::INTRHANDLER) * CIopBios::MAX_INTRHANDLER)
#define BIOS_HEAPBLOCK_BASE				(BIOS_INTRHANDLER_BASE + BIOS_INTRHANDLER_SIZE)
#define BIOS_HEAPBLOCK_SIZE				(sizeof(Iop::CSysmem::BLOCK) * Iop::CSysmem::MAX_BLOCKS)
#define BIOS_CALCULATED_END				(BIOS_HEAPBLOCK_BASE + BIOS_HEAPBLOCK_SIZE)

using namespace Framework;

CIopBios::CIopBios(uint32 clockFrequency, CMIPS& cpu, uint8* ram, uint32 ramSize) :
m_cpu(cpu),
m_ram(ram),
m_ramSize(ramSize),
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
m_threadFinishAddress(0),
m_clockFrequency(clockFrequency),
m_threads(reinterpret_cast<THREAD*>(&m_ram[BIOS_THREADS_BASE]), 1, MAX_THREAD),
m_semaphores(reinterpret_cast<SEMAPHORE*>(&m_ram[BIOS_SEMAPHORES_BASE]), 1, MAX_SEMAPHORE),
m_intrHandlers(reinterpret_cast<INTRHANDLER*>(&m_ram[BIOS_INTRHANDLER_BASE]), 1, MAX_INTRHANDLER)
{
	BOOST_STATIC_ASSERT(BIOS_CALCULATED_END <= CIopBios::CONTROL_BLOCK_END);
}

CIopBios::~CIopBios()
{
    DeleteModules();
}

void CIopBios::Reset(Iop::CSifMan* sifMan)
{
	//Assemble handlers
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[BIOS_HANDLERS_BASE]));
		m_threadFinishAddress = AssembleThreadFinish(assembler);
		m_returnFromExceptionAddress = AssembleReturnFromException(assembler);
		m_idleFunctionAddress = AssembleIdleFunction(assembler);
		assert(BIOS_HANDLERS_END > ((assembler.GetProgramSize() * 4) + BIOS_HANDLERS_BASE));
	}

	//0xBE00000 = Stupid constant to make FFX PSF happy
	CurrentTime() = 0xBE00000;
    ThreadLinkHead() = 0;
    CurrentThreadId() = -1;

    m_cpu.m_State.nCOP0[CCOP_SCU::STATUS] |= CMIPS::STATUS_INT;

    m_threads.FreeAll();
	m_semaphores.FreeAll();
    m_intrHandlers.FreeAll();
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
        m_sysmem = new Iop::CSysmem(m_ram, CONTROL_BLOCK_END, m_ramSize, BIOS_HEAPBLOCK_BASE, *m_stdio, *m_sifMan);
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
        m_cdvdman = new Iop::CCdvdman(m_ram);
        RegisterModule(m_cdvdman);
    }
	{
		RegisterModule(m_sifMan);
	}
    {
        m_sifCmd = new Iop::CSifCmd(*this, *m_sifMan, *m_sysmem, m_ram);
        RegisterModule(m_sifCmd);
    }
#ifdef _IOP_EMULATE_MODULES
    {
        RegisterModule(new Iop::CFileIo(*m_sifMan, *m_ioman));
    }
    {
        m_cdvdfsv = new Iop::CCdvdfsv(*m_sifMan, m_ram);
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
    //{
    //    RegisterModule(new Iop::CUnknown(*m_sifMan));
    //}
    //{
    //    RegisterModule(new Iop::CUnknown2(*m_sifMan));
    //}
#endif

    const int sifDmaBufferSize = 0x1000;
    uint32 sifDmaBufferPtr = m_sysmem->AllocateMemory(sifDmaBufferSize, 0);
#ifndef _NULL_SIFMAN
    m_sifMan->SetDmaBuffer(sifDmaBufferPtr, sifDmaBufferSize);
#endif

    Reschedule();
}

uint32& CIopBios::ThreadLinkHead() const
{
    return *reinterpret_cast<uint32*>(m_ram + BIOS_THREAD_LINK_HEAD_BASE);
}

uint32& CIopBios::CurrentThreadId() const
{
    return *reinterpret_cast<uint32*>(m_ram + BIOS_CURRENT_THREAD_ID_BASE);
}

uint64& CIopBios::CurrentTime() const
{
    return *reinterpret_cast<uint64*>(m_ram + BIOS_CURRENT_TIME_BASE);
}

void CIopBios::SaveState(CZipArchiveWriter& archive)
{
    CStructCollectionStateFile* modulesFile = new CStructCollectionStateFile(STATE_MODULES);
    {
        for(DynamicIopModuleListType::iterator moduleIterator(m_dynamicModules.begin());
            moduleIterator != m_dynamicModules.end(); moduleIterator++)
        {
            Iop::CDynamic* module(*moduleIterator);
            CStructFile moduleStruct;
            {
                uint32 importTableAddress = reinterpret_cast<uint8*>(module->GetExportTable()) - m_ram;
                moduleStruct.SetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS, importTableAddress); 
            }
            modulesFile->InsertStruct(module->GetId().c_str(), moduleStruct);
        }
    }
    archive.InsertFile(modulesFile);

    m_sifCmd->SaveState(archive);
}

void CIopBios::LoadState(CZipArchiveReader& archive)
{
    ClearDynamicModules();

    CStructCollectionStateFile modulesFile(*archive.BeginReadFile(STATE_MODULES));
    {
        for(CStructCollectionStateFile::StructIterator structIterator(modulesFile.GetStructBegin());
            structIterator != modulesFile.GetStructEnd(); structIterator++)
        {
            const CStructFile& structFile(structIterator->second);
            uint32 importTableAddress = structFile.GetRegister32(STATE_MODULE_IMPORT_TABLE_ADDRESS);
            Iop::CDynamic* module = new Iop::CDynamic(reinterpret_cast<uint32*>(m_ram + importTableAddress));
            RegisterDynamicModule(module);
        }
    }

    m_sifCmd->LoadState(archive);
}

bool CIopBios::IsIdle()
{
	return (m_cpu.m_State.nPC == m_idleFunctionAddress);
}

void CIopBios::LoadAndStartModule(const char* path, const char* args, unsigned int argsLength)
{
    uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::O_RDONLY, path);
    if(handle & 0x80000000)
    {
		CLog::GetInstance().Print(LOGNAME, "Tried to load '%s' which couldn't be found.", path);
		return;
    }
    Iop::CIoman::CFile file(handle, *m_ioman);
    CStream* stream = m_ioman->GetFileStream(file);
    CElfFile module(*stream);
    LoadAndStartModule(module, path, args, argsLength);
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

    //Find .iopmod section
    const ELFHEADER& header(elf.GetHeader());
    const IOPMOD* iopMod = NULL;
    for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
    {
        ELFSECTIONHEADER* sectionHeader(elf.GetSection(i));
        if(sectionHeader->nType != IOPMOD_SECTION_ID) continue;
        iopMod = reinterpret_cast<const IOPMOD*>(elf.GetSectionData(i));
    }

    ModuleListIterator moduleIterator(FindModule(moduleRange.first, moduleRange.second));
    if(moduleIterator == m_moduleTags.end())
    {
        MIPSMODULE module;
        module.name  = GetModuleNameFromPath(path);
        module.begin = moduleRange.first;
        module.end   = moduleRange.second;
        m_moduleTags.push_back(module);
    }

    uint32 threadId = CreateThread(entryPoint, DEFAULT_PRIORITY, DEFAULT_STACKSIZE);
    THREAD& thread = GetThread(threadId);

    typedef std::vector<uint32> ParamListType;
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
            unsigned int argLength = static_cast<unsigned int>(strlen(arg)) + 1;
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
    if(iopMod != NULL)
    {
        thread.context.gpr[CMIPS::GP] = iopMod->gp + moduleRange.first;
    }

    StartThread(threadId);
    if(CurrentThreadId() == -1)
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
			std::string moduleName = ReadModuleName(address + 0xC);
	        IopModuleMapType::iterator module(m_modules.find(moduleName));

			size_t moduleNameLength = moduleName.length();
			uint32 entryAddress = address + 0x0C + ((moduleNameLength + 3) & ~0x03);
			while(m_cpu.m_pMemoryMap->GetWord(entryAddress) == 0x03E00008)
			{
				uint32 target = m_cpu.m_pMemoryMap->GetWord(entryAddress + 4);
				uint32 functionId = target & 0xFFFF;
				std::string functionName = "unknown";
                if(module != m_modules.end())
                {
				    functionName = (module->second)->GetFunctionName(functionId);
                }
				if(m_cpu.m_Functions.Find(address) == NULL)
				{
					m_cpu.m_Functions.InsertTag(entryAddress, (std::string(moduleName) + "_" + functionName).c_str());
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
    return *m_threads[threadId];
}

uint32 CIopBios::CreateThread(uint32 threadProc, uint32 priority, uint32 stackSize)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: CreateThread(threadProc = 0x%0.8X, priority = %d);\r\n", 
        CurrentThreadId(), threadProc, priority);
#endif

    if(stackSize == 0)
    {
        stackSize = DEFAULT_STACKSIZE;
    }

	uint32 threadId = m_threads.Allocate();
	assert(threadId != -1);
	if(threadId == -1)
	{
		return -1;
	}

    THREAD* thread = m_threads[threadId];
    memset(&thread->context, 0, sizeof(thread->context));
    thread->context.delayJump = 1;
	thread->stackSize = stackSize;
    thread->stackBase = m_sysmem->AllocateMemory(thread->stackSize, 0);
    memset(m_ram + thread->stackBase, 0, thread->stackSize);
	thread->id = threadId;
    thread->priority = priority;
    thread->status = THREAD_STATUS_CREATED;
    thread->context.epc = threadProc;
    thread->nextActivateTime = 0;
    thread->context.gpr[CMIPS::RA] = m_threadFinishAddress;
    thread->context.gpr[CMIPS::SP] = thread->stackBase + thread->stackSize;
    thread->context.gpr[CMIPS::GP] = m_cpu.m_State.nGPR[CMIPS::GP].nV0;
	LinkThread(thread->id);
    return thread->id;
}

void CIopBios::DeleteThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	UnlinkThread(threadId);
	m_sysmem->FreeMemory(thread->stackBase);
	m_threads.Free(threadId);
}

void CIopBios::StartThread(uint32 threadId, uint32* param)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: StartThread(threadId = %i, param = 0x%0.8X);\r\n", 
        CurrentThreadId(), threadId, param);
#endif

    THREAD& thread = GetThread(threadId);
    if(thread.status != THREAD_STATUS_CREATED)
    {
        throw std::runtime_error("Invalid thread state.");
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
        CurrentThreadId(), delay);
#endif

    //TODO : Need to delay or something...
    THREAD& thread = GetThread(CurrentThreadId());
    thread.nextActivateTime = GetCurrentTime() + MicroSecToClock(delay);
    m_rescheduleNeeded = true;
}

void CIopBios::SleepThread()
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: SleepThread();\r\n", 
        CurrentThreadId());
#endif

    THREAD& thread = GetThread(CurrentThreadId());
    if(thread.status != THREAD_STATUS_RUNNING)
    {
        throw std::runtime_error("Thread isn't running.");
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
        CurrentThreadId(), threadId);
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
    return CurrentThreadId();
}

void CIopBios::ExitCurrentThread()
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%d : ExitCurrentThread();\r\n", CurrentThreadId());
#endif
	DeleteThread(CurrentThreadId());
    CurrentThreadId() = -1;
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

void CIopBios::LinkThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			(*nextThreadId) = threadId;
			thread->nextThreadId = 0;
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
		if(currentThread->priority < thread->priority)
		{
			thread->nextThreadId = (*nextThreadId);
			(*nextThreadId) = threadId;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

void CIopBios::UnlinkThread(uint32 threadId)
{
	THREAD* thread = m_threads[threadId];
	uint32* nextThreadId = &ThreadLinkHead();
	while(1)
	{
		if((*nextThreadId) == 0)
		{
			break;
		}
		THREAD* currentThread = m_threads[(*nextThreadId)];
		if((*nextThreadId) == threadId)
		{
			(*nextThreadId) = thread->nextThreadId;
			thread->nextThreadId = 0;
			break;
		}
		nextThreadId = &currentThread->nextThreadId;
	}
}

void CIopBios::Reschedule()
{
    if(CurrentThreadId() != -1)
    {
        SaveThreadContext(CurrentThreadId());
		UnlinkThread(CurrentThreadId());
		LinkThread(CurrentThreadId());
    }

    uint32 nextThreadId = GetNextReadyThread();
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
	if(nextThreadId != CurrentThreadId())
	{
		CLog::GetInstance().Print(LOGNAME, "Switched over to thread %i.\r\n", nextThreadId);
	}
#endif
	CurrentThreadId() = nextThreadId;
	m_cpu.m_nQuota = 1;
}

uint32 CIopBios::GetNextReadyThread()
{
	uint32 nextThreadId = ThreadLinkHead();
	while(nextThreadId != 0)
	{
        THREAD* nextThread = m_threads[nextThreadId];
		nextThreadId = nextThread->nextThreadId;
        if(GetCurrentTime() <= nextThread->nextActivateTime) continue;
        if(nextThread->status == THREAD_STATUS_RUNNING)
        {
            return nextThread->id;
        }
	}
	return -1;
}

uint64 CIopBios::GetCurrentTime()
{
	return CurrentTime();
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
	CurrentTime() += ticks;
}

uint32 CIopBios::CreateSemaphore(uint32 initialCount, uint32 maxCount)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: CreateSemaphore(initialCount = %i, maxCount = %i);\r\n", 
        CurrentThreadId(), initialCount, maxCount);
#endif

	uint32 semaphoreId = m_semaphores.Allocate();
	assert(semaphoreId != -1);
	if(semaphoreId == -1)
	{
		return -1;
	}

    SEMAPHORE* semaphore = m_semaphores[semaphoreId];

	semaphore->count		= initialCount;
    semaphore->maxCount		= maxCount;
    semaphore->id			= semaphoreId;
    semaphore->waitCount	= 0;

	return semaphore->id;
}

uint32 CIopBios::DeleteSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: DeleteSemaphore(semaphoreId = %i);\r\n",
        CurrentThreadId(), semaphoreId);
#endif

    SEMAPHORE* semaphore = m_semaphores[semaphoreId];
    if(semaphore == NULL)
    {
		CLog::GetInstance().Print(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
			CurrentThreadId(), semaphoreId);
        return -1;
    }

    assert(semaphore->waitCount == 0);
    m_semaphores.Free(semaphoreId);

    return 0;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId, bool inInterrupt)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: SignalSemaphore(semaphoreId = %i);\r\n", 
        CurrentThreadId(), semaphoreId);
#endif

    SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
			CurrentThreadId(), semaphoreId);
		return -1;
	}

    if(semaphore->waitCount != 0)
    {
		for(ThreadList::iterator threadIterator(m_threads.Begin());
			threadIterator != m_threads.End(); threadIterator++)
		{
            THREAD* thread(m_threads[threadIterator]);
			if(thread == NULL) continue;
            if(thread->waitSemaphore == semaphoreId)
            {
                if(thread->status != THREAD_STATUS_WAITING)
                {
                    throw std::runtime_error("Thread not waiting (inconsistent state).");
                }
                thread->status = THREAD_STATUS_RUNNING;
                thread->waitSemaphore = 0;
				if(!inInterrupt)
				{
					m_rescheduleNeeded = true;
				}
                semaphore->waitCount--;
                if(semaphore->waitCount == 0)
                {
                    break;
                }
            }
        }
    }
    else
    {
        semaphore->count++;
    }
    return semaphore->count;
}

uint32 CIopBios::WaitSemaphore(uint32 semaphoreId)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "%i: WaitSemaphore(semaphoreId = %i);\r\n", 
        CurrentThreadId(), semaphoreId);
#endif

    SEMAPHORE* semaphore = m_semaphores[semaphoreId];
	if(semaphore == NULL)
	{
		CLog::GetInstance().Print(LOGNAME, "%i: Warning, trying to access invalid semaphore with id %i.\r\n",
			CurrentThreadId(), semaphoreId);
		return -1;
	}

    if(semaphore->count == 0)
    {
        THREAD& thread = GetThread(CurrentThreadId());
        thread.status           = THREAD_STATUS_WAITING;
        thread.waitSemaphore    = semaphoreId;
        semaphore->waitCount++;
        m_rescheduleNeeded      = true;
    }
    else
    {
        semaphore->count--;
    }
    return semaphore->count;
}

Iop::CIoman* CIopBios::GetIoman()
{
    return m_ioman;
}

Iop::CCdvdman* CIopBios::GetCdvdman()
{
    return m_cdvdman;
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
	assert(FindIntrHandler(line) == -1);

	uint32 handlerId = m_intrHandlers.Allocate();
	assert(handlerId != -1);
	if(handlerId == -1)
	{
		return false;
	}

    INTRHANDLER* intrHandler = m_intrHandlers[handlerId];
    intrHandler->line		= line;
    intrHandler->mode		= mode;
    intrHandler->handler	= handler;
    intrHandler->arg		= arg;

    return true;
}

bool CIopBios::ReleaseIntrHandler(uint32 line)
{
	uint32 handlerId = FindIntrHandler(line);
	if(handlerId == -1)
	{
		return false;
	}
	m_intrHandlers.Free(handlerId);
    return true;
}

uint32 CIopBios::FindIntrHandler(uint32 line)
{
	for(IntrHandlerList::iterator handlerIterator(m_intrHandlers.Begin());
		handlerIterator != m_intrHandlers.End(); handlerIterator++)
	{
		INTRHANDLER* handler = m_intrHandlers[handlerIterator];
		if(handler == NULL) continue;
		if(handler->line == line) return handlerIterator;
	}
	return -1;
}

uint32 CIopBios::AssembleThreadFinish(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0666);
    assembler.SYSCALL();
    return address;
}

uint32 CIopBios::AssembleReturnFromException(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0667);
    assembler.SYSCALL();
    return address;
}

uint32 CIopBios::AssembleIdleFunction(CMIPSAssembler& assembler)
{
    uint32 address = BIOS_HANDLERS_BASE + assembler.GetProgramSize() * 4;
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
		std::string moduleName = ReadModuleName(searchAddress + 0x0C);

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
			uint32 handlerId = FindIntrHandler(line);
			if(handlerId == -1)
			{
				ReturnFromException();
				return;
			}
			else
			{
				//Snap out of current thread
				if(CurrentThreadId() != -1)
				{
					SaveThreadContext(CurrentThreadId());
				}
				CurrentThreadId() = -1;
				INTRHANDLER* handler = m_intrHandlers[handlerId];
				m_cpu.m_State.nPC = handler->handler;
				m_cpu.m_State.nGPR[CMIPS::A0].nD0 = static_cast<int32>(handler->arg);
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

std::string CIopBios::GetModuleNameFromPath(const std::string& path)
{
	std::string::size_type slashPosition;
    slashPosition = path.rfind('/');
    if(slashPosition != std::string::npos)
    {
        return std::string(path.begin() + slashPosition + 1, path.end());
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
		module.begin    = lexical_cast_hex<std::string>(beginAddress);
        module.end      = lexical_cast_hex<std::string>(endAddress);
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
		moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_BEGINADDRESS,   lexical_cast_hex<std::string>(module.begin, 8).c_str());
        moduleNode->InsertAttribute(TAGS_SECTION_IOP_MODULES_MODULE_ENDADDRESS,     lexical_cast_hex<std::string>(module.end, 8).c_str());
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

void CIopBios::DeleteModules()
{
    while(m_modules.size() != 0)
    {
        delete m_modules.begin()->second;
        m_modules.erase(m_modules.begin());
    }
    m_dynamicModules.clear();

    m_sifMan = NULL;
    m_stdio = NULL;
    m_ioman = NULL;
    m_sysmem = NULL;
    m_modload = NULL;
    m_sysmem = NULL;
#ifdef _IOP_EMULATE_MODULES
    m_dbcman = NULL;
    m_padman = NULL;
    m_cdvdfsv = NULL;
#endif
}

std::string CIopBios::ReadModuleName(uint32 address)
{
	std::string moduleName;
	CMemoryMap::MEMORYMAPELEMENT* memoryMapElem = m_cpu.m_pMemoryMap->GetReadMap(address);
	assert(memoryMapElem != NULL);
	assert(memoryMapElem->nType == CMemoryMap::MEMORYMAP_TYPE_MEMORY);
	uint8* memory = reinterpret_cast<uint8*>(memoryMapElem->pPointer) + (address - memoryMapElem->nStart);
	while(1)
	{
		uint8 character = *(memory++);
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

void CIopBios::ClearDynamicModules()
{
    for(DynamicIopModuleListType::iterator dynModuleIterator(m_dynamicModules.begin());
        dynModuleIterator != m_dynamicModules.end(); dynModuleIterator++)
    {
        IopModuleMapType::iterator module(m_modules.find((*dynModuleIterator)->GetId()));
        assert(module != m_modules.end());
        if(module != m_modules.end())
        {
            delete module->second;
            m_modules.erase(module);
        }
    }

    m_dynamicModules.clear();
}

void CIopBios::RegisterDynamicModule(Iop::CDynamic* dynamicModule)
{
    m_dynamicModules.push_back(dynamicModule);
    RegisterModule(dynamicModule);
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
        throw std::runtime_error("No program to load.");
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
                throw std::runtime_error("Multiple loadable program headers found.");
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
                uint32 relocationType = relocationRecord[1] & 0xFF;
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
						throw std::runtime_error("Unknown relocation type.");
                        break;
                    }
                }
                relocationRecord += 2;
            }
        }
    }
}
