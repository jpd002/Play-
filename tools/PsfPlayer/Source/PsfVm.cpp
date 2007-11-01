#include "PsfVm.h"
#include "MA_MIPSIV.h"
#include "PtrStream.h"
#include "ELF.h"

using namespace Framework;
using namespace std;

CPsfVm::CPsfVm(const char* psfPath) :
m_fileSystem(psfPath),
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0x00000000, RAMSIZE)
{
    //Initialize block map
    m_blockMap[RAMSIZE] = 0;

    m_ram = new uint8[RAMSIZE];
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);
    m_cpu.m_pArch = &g_MAMIPSIV;
    m_cpu.m_pAddrTranslator = m_cpu.TranslateAddress64;
    m_cpu.m_pTickFunction = TickFunction;

    uint32 entryPoint = LoadIopModule("psf2.irx", 0x000100000);
    m_cpu.m_State.nPC = entryPoint;
}

CPsfVm::~CPsfVm()
{
    delete [] m_ram;
}

CMIPS& CPsfVm::GetCpu()
{
    return m_cpu;
}

uint32 CPsfVm::LoadIopModule(const char* modulePath, uint32 baseAddress)
{
    const CPsfFs::FILE* file = m_fileSystem.GetFile(modulePath);
    CPtrStream stream(file->data, file->size);
    CELF elf(&stream);

//    baseAddress = AllocateMemory(elf.m_nLenght);
    baseAddress = 0x1B000;

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

unsigned int CPsfVm::TickFunction(unsigned int dummy)
{
    return 1;
}

uint32 CPsfVm::AllocateMemory(uint32 size)
{
    uint32 begin = 0x1000;
    const uint32 blockSize = 0x400;
    size = ((size + (blockSize - 1)) / blockSize) * blockSize;
    for(BlockMapType::iterator blockIterator(m_blockMap.begin());
        blockIterator != m_blockMap.end(); blockIterator++)
    {
        uint32 end = blockIterator->first;
        if((end - begin) >= size)
        {
            break;
        }
        begin = blockIterator->first + blockIterator->second;
    }
    if(begin != RAMSIZE)
    {
        m_blockMap[begin] = size;
        return begin;
    }
    return NULL;
}

void CPsfVm::FreeMemory(uint32 address)
{
    BlockMapType::iterator block(m_blockMap.find(address));
    if(block != m_blockMap.end())
    {
        m_blockMap.erase(block);
    }
    else
    {
        printf("%s: Trying to unallocate an unexisting memory block (0x%0.8X).\r\n", __FUNCTION__, address);
    }
}
