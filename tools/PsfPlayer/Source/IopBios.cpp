#include "IopBios.h"
#include "COP_SCU.h"
#include "Iop_Sysclib.h"
#include "Iop_Loadcore.h"
#include <vector>

using namespace std;
using namespace Framework;

CIopBios::CIopBios(uint32 baseAddress, CMIPS& cpu, uint8* ram, uint32 ramSize) :
m_baseAddress(baseAddress),
m_cpu(cpu),
m_ram(ram),
m_nextThreadId(1),
m_stdio(NULL),
m_sysmem(NULL),
m_ioman(NULL),
m_modload(NULL),
m_currentThreadId(-1)
{
    CMIPSAssembler assembler(reinterpret_cast<uint32*>(&m_ram[m_baseAddress]));
    m_threadFinishAddress = AssembleThreadFinish(assembler);

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
        m_sysmem = new Iop::CSysmem(0x1000, ramSize);
        RegisterModule(m_sysmem);
    }
    {
        m_modload = new Iop::CModload(*this, m_ram);
        RegisterModule(m_modload);
    }
    {
        RegisterModule(new Iop::CSysclib(m_ram));
    }
    {
        RegisterModule(new Iop::CLoadcore(*this, m_ram));
    }
}

CIopBios::~CIopBios()
{
    while(m_modules.size() != 0)
    {
        delete m_modules.begin()->second;
        m_modules.erase(m_modules.begin());
    }
}

void CIopBios::LoadAndStartModule(const char* path, const char* args, unsigned int argsLength)
{
    uint32 entryPoint = LoadExecutable(path);
    uint32 threadId = CreateThread(entryPoint);
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
    thread.context.gpr[CMIPS::A0] = paramList.size();
    for(ParamListType::reverse_iterator param(paramList.rbegin());
        paramList.rend() != param; param++)
    {
        thread.context.gpr[CMIPS::A1] = Push(
            thread.context.gpr[CMIPS::SP],
            reinterpret_cast<const uint8*>(&(*param)),
            4);
    }

    Reschedule();
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

uint32 CIopBios::CreateThread(uint32 threadProc)
{
    THREAD thread;
    memset(&thread, 0, sizeof(thread));
    thread.context.delayJump = 1;
    uint32 stackBaseAddress = m_sysmem->AllocateMemory(DEFAULT_STACKSIZE, 0);
    thread.id = m_nextThreadId++;
    thread.priority = 7;
    thread.context.epc = threadProc;
    thread.context.gpr[CMIPS::RA] = m_threadFinishAddress;
    thread.context.gpr[CMIPS::SP] = stackBaseAddress;
    m_threads.insert(ThreadMapType::value_type(thread.priority, thread));
    return thread.id;
}

void CIopBios::ExitCurrentThread()
{
    ThreadMapType::iterator thread = GetThreadPosition(m_currentThreadId);
    m_threads.erase(thread);
    m_currentThreadId = -1;
    Reschedule();
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

    THREAD& nextThread = m_threads.begin()->second;
    LoadThreadContext(nextThread.id);
    m_currentThreadId = nextThread.id;
    m_cpu.m_nQuota = 1;
}

Iop::CIoman* CIopBios::GetIoman()
{
    return m_ioman;
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
            printf("IOP(%0.8X): Trying to call a function from non-existing module (%s, %d).\r\n", 
                m_cpu.m_State.nPC, moduleName.c_str(), functionId);
        }
    }

    m_cpu.m_State.nCOP0[CCOP_SCU::EPC] = 0;
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

uint32 CIopBios::LoadExecutable(const char* path)
{
    uint32 handle = m_ioman->Open(Iop::Ioman::CDevice::O_RDONLY, path);
    if(handle & 0x80000000)
    {
        throw runtime_error("Couldn't open executable for reading.");
    }
    Iop::CIoman::CFile file(handle, *m_ioman);
    CStream* stream = m_ioman->GetFileStream(file);
    CELF elf(stream);

    uint32 baseAddress = m_sysmem->AllocateMemory(elf.m_nLenght, 0);
    RelocateElf(elf, baseAddress);

    for(unsigned int i = 0; i < elf.m_Header.nProgHeaderCount; i++)
    {
        ELFPROGRAMHEADER* programHeader = elf.GetProgram(i);
        if(programHeader != NULL && programHeader->nType == 1)
        {
            memcpy(
                m_ram + baseAddress, 
                elf.m_pData + programHeader->nOffset, 
                programHeader->nFileSize);
        }
    }

    return baseAddress + elf.m_Header.nEntryPoint;
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
                                printf("%s: No HI16 relocation record found for corresponding LO16.\r\n", __FUNCTION__);
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
