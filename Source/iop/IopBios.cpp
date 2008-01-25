#include "IopBios.h"
#include "../COP_SCU.h"
#include "../Log.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include "Iop_LibSd.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_McServ.h"
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
m_iso(iso),
m_nextThreadId(1),
m_nextSemaphoreId(1),
m_stdio(NULL),
m_sysmem(NULL),
m_ioman(NULL),
m_modload(NULL),
m_dbcman(NULL),
m_rescheduleNeeded(false),
m_currentThreadId(-1)
{
    CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[m_baseAddress]));
    m_threadFinishAddress = AssembleThreadFinish(assembler);
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
    DeleteModules();

    //Register built-in modules
    {
        m_stdio = new Iop::CStdio(m_ram);
        RegisterModule(m_stdio);
    }
    {
        m_ioman = new Iop::CIoman(m_ram, m_sif);
        RegisterModule(m_ioman);
    }
    {
        m_sysmem = new Iop::CSysmem(0x1000, m_ramSize, *m_stdio, m_sif);
        RegisterModule(m_sysmem);
    }
    {
        m_modload = new Iop::CModload(*this, m_ram);
        RegisterModule(m_modload);
    }
    {
        RegisterModule(new Iop::CCdvdfsv(m_iso, m_sif));
    }
    {
        RegisterModule(new Iop::CSysclib(m_ram, *m_stdio));
    }
    {
        RegisterModule(new Iop::CLoadcore(*this, m_ram, m_sif));
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
        RegisterModule(new Iop::CTimrman());
    }
    {
        RegisterModule(new Iop::CIntrman(m_ram));
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

    const int sifDmaBufferSize = 0x1000;
    uint32 sifDmaBufferPtr = m_sysmem->AllocateMemory(sifDmaBufferSize, 0);
    m_sif.SetDmaBuffer(m_ram + sifDmaBufferPtr, sifDmaBufferSize);
}

void CIopBios::LoadAndStartModule(const char* path, const char* args, unsigned int argsLength)
{
    ExecutableRange moduleRange;
    uint32 entryPoint = LoadExecutable(path, moduleRange);

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
        paramList.push_back(Push(
            thread.context.gpr[CMIPS::SP],
            reinterpret_cast<const uint8*>(args),
            static_cast<uint32>(strlen(args)) + 1));
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
    LoadModuleTags(loadedModule, m_cpu.m_Comments, "comments");
    LoadModuleTags(loadedModule, m_cpu.m_Functions, "functions");
    m_cpu.m_pAnalysis->Analyse(moduleRange.first, moduleRange.second);
#endif

	for(int i = 0; i < 0x00400000 / 4; i++)
	{
		uint32 nVal = ((uint32*)m_ram)[i];
//        if(nVal == 0x34C00)
//        {
//			printf("Allo: 0x%0.8X\r\n", i * 4);
//        }
/*
        if((nVal & 0xFFFF) == 0xB730)
        {
            char mnemonic[256];
            m_cpu.m_pArch->GetInstructionMnemonic(&m_cpu, i * 4, nVal, mnemonic, 256);
            printf("Allo: %s, 0x%0.8X\r\n", mnemonic, i * 4);
        }
*/
/*
        if(nVal == 0x2F9B50)
		{
			printf("Allo: 0x%0.8X\r\n", i * 4);
		}
*/

        if((nVal & 0xFC000000) == 0x0C000000)
		{
			nVal &= 0x3FFFFFF;
			nVal *= 4;
			if(nVal == 0x41410)
			{
				printf("Allo: 0x%0.8X\r\n", i * 4);
			}
		}

	}

    *reinterpret_cast<uint32*>(&m_ram[0x41674]) = 0;
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
    //TODO : Need to delay or something...
    THREAD& thread = GetThread(m_currentThreadId);
    thread.nextActivateTime = GetCurrentTime() + delay;
    m_rescheduleNeeded = true;
}

void CIopBios::SleepThread()
{
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
        if(nextThreadId == -1)
        {
            throw runtime_error("No thread available for running.");
        }
    }

    LoadThreadContext(nextThreadId);
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
    SEMAPHORE semaphore;
    memset(&semaphore, 0, sizeof(SEMAPHORE));
    semaphore.count = initialCount;
    semaphore.maxCount = maxCount;
    semaphore.id = m_nextSemaphoreId++;
    semaphore.waitCount = 0;
    m_semaphores[semaphore.id] = semaphore;
    return semaphore.id;
}

uint32 CIopBios::SignalSemaphore(uint32 semaphoreId)
{
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
                m_rescheduleNeeded = true;
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

Iop::CDbcMan* CIopBios::GetDbcman()
{
    return m_dbcman;
}

uint32 CIopBios::AssembleThreadFinish(CMIPSAssembler& assembler)
{
    uint32 address = m_baseAddress + assembler.GetProgramSize();
    assembler.ADDIU(CMIPS::V0, CMIPS::R0, 0x0666);
    assembler.SYSCALL();
    return address;
}

void CIopBios::SysCallHandler()
{
    uint32 searchAddress = m_cpu.m_State.nCOP0[CCOP_SCU::EPC];
    uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
    if(callInstruction == 0x0000000C)
    {
        if(m_cpu.m_State.nGPR[CMIPS::V0].nV0 == 0x666)
        {
            ExitCurrentThread();
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
        m_rescheduleNeeded = false;
        Reschedule();
    }

    m_cpu.m_State.nHasException = 0;
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

uint32 CIopBios::LoadExecutable(const char* path, ExecutableRange& executableRange)
{
    uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::O_RDONLY, path);
    if(handle & 0x80000000)
    {
        throw runtime_error("Couldn't open executable for reading.");
    }
    Iop::CIoman::CFile file(handle, *m_ioman);
    CStream* stream = m_ioman->GetFileStream(file);
    CELF elf(stream);

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
        elf.m_pData + programHeader->nOffset, 
        programHeader->nFileSize);

    executableRange.first = baseAddress;
    executableRange.second = baseAddress + programHeader->nMemorySize;
    return baseAddress + elf.m_Header.nEntryPoint;
}

unsigned int CIopBios::GetElfProgramToLoad(CELF& elf)
{
    unsigned int program = -1;
    for(unsigned int i = 0; i < elf.m_Header.nProgHeaderCount; i++)
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
    for(unsigned int i = 0; i < elf.m_Header.nSectHeaderCount; i++)
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
