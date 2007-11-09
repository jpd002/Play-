#include "PsfVm.h"
#include "MA_MIPSIV.h"
#include "PtrStream.h"
#include "ELF.h"
#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>

using namespace Framework;
using namespace std;
using namespace boost;

CPsfVm::CPsfVm(const char* psfPath) :
m_fileSystem(psfPath),
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0x00000000, RAMSIZE),
m_status(PAUSED),
m_pauseAck(false),
m_emuThread(NULL),
m_iopStdio(NULL)
{
    memset(&m_cpu.m_State, 0, sizeof(m_cpu.m_State));
    m_ram = new uint8[RAMSIZE];
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);
    m_cpu.m_pMemoryMap->InsertWriteMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);

    m_cpu.m_pArch = &g_MAMIPSIV;
    m_cpu.m_pAddrTranslator = m_cpu.TranslateAddress64;
    m_cpu.m_pTickFunction = reinterpret_cast<TickFunctionType>(TickFunctionStub);
    m_cpu.m_pSysCallHandler = reinterpret_cast<SysCallHandlerType>(SysCallHandlerStub);
    m_cpu.m_handlerParam = this;

#ifdef _DEBUG
    m_cpu.m_Functions.Unserialize("functions.bin");
    m_cpu.m_Comments.Unserialize("comments.bin");
#endif

    //Register built-in modules
    {
        m_iopStdio = new Iop::CStdio(m_ram);
        RegisterModule(m_iopStdio);
    }
    {
        m_iopIoman = new Iop::CIoman(m_ram);
        RegisterModule(m_iopIoman);
        m_iopIoman->RegisterDevice("host", new Iop::Ioman::CPsf(m_fileSystem));
    }
    {
        m_iopSysmem = new Iop::CSysmem(0x1000, RAMSIZE);
        RegisterModule(m_iopSysmem);
    }

    uint32 stackBegin = m_iopSysmem->AllocateMemory(DEFAULT_STACKSIZE, 0);
    uint32 entryPoint = LoadIopModule("psf2.irx", 0x000100000);
    string execPath = "host:/psf2.irx";
    m_cpu.m_State.nGPR[CMIPS::SP].nV0 = stackBegin + DEFAULT_STACKSIZE;
    m_cpu.m_State.nGPR[CMIPS::A0].nV0 = 1;
    uint32 firstParam = Push(
        m_cpu.m_State.nGPR[CMIPS::SP].nV0,
        reinterpret_cast<const uint8*>(execPath.c_str()),
        execPath.length() + 1);
    m_cpu.m_State.nGPR[CMIPS::A1].nV0 = Push(
        m_cpu.m_State.nGPR[CMIPS::SP].nV0,
        reinterpret_cast<const uint8*>(&firstParam),
        4);
    m_cpu.m_State.nPC = entryPoint;


    m_emuThread = new thread(bind(&CPsfVm::EmulationProc, this));
}

CPsfVm::~CPsfVm()
{
#ifdef _DEBUG
    m_cpu.m_Functions.Serialize("functions.bin");
    m_cpu.m_Comments.Serialize("comments.bin");
#endif

    while(m_iopModules.size() != 0)
    {
        delete m_iopModules.begin()->second;
        m_iopModules.erase(m_iopModules.begin());
    }
    delete [] m_ram;
}

void CPsfVm::Pause()
{
    if(GetStatus() == PAUSED) return;
    m_pauseAck = false;
    m_status = PAUSED;
    while(!m_pauseAck)
    {
        xtime xt;
        xtime_get(&xt, boost::TIME_UTC);
        xt.nsec += 100000000;
    }
    m_OnRunningStateChange();
}

void CPsfVm::Resume()
{
    if(GetStatus() == RUNNING) return;
    m_status = RUNNING;
    m_OnRunningStateChange();
}

CMIPS& CPsfVm::GetCpu()
{
    return m_cpu;
}

CVirtualMachine::STATUS CPsfVm::GetStatus() const
{
    return m_status;
}

uint32 CPsfVm::LoadIopModule(const char* modulePath, uint32 baseAddress)
{
    const CPsfFs::FILE* file = m_fileSystem.GetFile(modulePath);
    CPtrStream stream(file->data, file->size);
    CELF elf(&stream);

    baseAddress = m_iopSysmem->AllocateMemory(elf.m_nLenght, 0);

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
            for(unsigned int record = 0; record < recordCount; record++)
            {
                uint32 relocationAddress = relocationRecord[0];
                uint32 relocationType = relocationRecord[1];
                if(relocationAddress < relocatedSection->nSize) 
                {
                    uint32& instruction = *reinterpret_cast<uint32*>(&relocatedSectionData[relocationAddress]);
                    switch(relocationType)
                    {
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
                        throw exception();
                        break;
                    }
                }
                relocationRecord += 2;
            }
        }
    }

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

unsigned int CPsfVm::TickFunctionStub(unsigned int ticks, CMIPS* context)
{
    return reinterpret_cast<CPsfVm*>(context->m_handlerParam)->TickFunction(ticks);
}

unsigned int CPsfVm::TickFunction(unsigned int ticks)
{
    if(m_cpu.MustBreak())
    {
        return 1;
    }
    return 0;
}

string CPsfVm::ReadModuleName(uint32 address)
{
    string moduleName;
    while(1)
    {
        uint8 character = m_cpu.m_pMemoryMap->GetByte(address);
        if(character == 0) break;
        moduleName += character;
        address++;
    }
    return moduleName;
}

void CPsfVm::RegisterModule(Iop::CModule* module)
{
    m_iopModules[module->GetId()] = module;
}

void CPsfVm::SysCallHandlerStub(CMIPS* state)
{
    reinterpret_cast<CPsfVm*>(state->m_handlerParam)->SysCallHandler();
}

void CPsfVm::SysCallHandler()
{
    uint32 searchAddress = m_cpu.m_State.nPC - 4;
    uint32 callInstruction = m_cpu.m_pMemoryMap->GetWord(searchAddress);
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

    IopModuleMapType::iterator module(m_iopModules.find(moduleName));
    if(module != m_iopModules.end())
    {
        module->second->Invoke(m_cpu, functionId);
    }
    else
    {
        printf("IOP(%0.8X): Trying to call a function from non-existing module (%s, %d).\r\n", 
            m_cpu.m_State.nPC, moduleName.c_str(), functionId);
    }
}

uint32 CPsfVm::Push(uint32& address, const uint8* data, uint32 size)
{
    uint32 fixedSize = ((size + 0xF) / 0x10) * 0x10;
    address -= fixedSize;
    memcpy(&m_ram[address], data, size);
    return address;
}

void CPsfVm::EmulationProc()
{
    while(1)
    {
        if(m_status == RUNNING)
        {
            RET_CODE returnCode = m_cpu.Execute(1000);
            if(returnCode == RET_CODE_BREAKPOINT)
            {
                m_status = PAUSED;
                m_OnRunningStateChange();
                m_OnMachineStateChange();
            }
        }
        else
        {
            m_pauseAck = true;
            xtime xt;
            xtime_get(&xt, boost::TIME_UTC);
            xt.nsec += 100000000;
            thread::sleep(xt);
        }
    }
}
