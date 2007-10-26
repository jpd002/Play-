#include "PsfVm.h"
#include "MA_MIPSIV.h"
#include "PtrStream.h"
#include "ELF.h"

using namespace Framework;

CPsfVm::CPsfVm(const char* psfPath) :
m_fileSystem(psfPath),
m_cpu(MEMORYMAP_ENDIAN_LSBF, 0x00000000, RAMSIZE)
{
    m_ram = new uint8[RAMSIZE];
	m_cpu.m_pMemoryMap->InsertReadMap(0x00000000, RAMSIZE - 1, m_ram, MEMORYMAP_TYPE_MEMORY, 0x00);
    m_cpu.m_pArch = &g_MAMIPSIV;

    uint32 entryPoint = LoadIopModule("psf2.irx", 0x000100000);
/*
    for(unsigned int i = 0; i < 100; i++)
    {
        char mnemonic[256];
        char operands[256];
        uint32 opcode = m_cpu.m_pMemoryMap->GetWord(entryPoint);
        m_cpu.m_pArch->GetInstructionMnemonic(&m_cpu, entryPoint, opcode, mnemonic, _countof(mnemonic));
        m_cpu.m_pArch->GetInstructionOperands(&m_cpu, entryPoint, opcode, operands, _countof(operands));
        entryPoint += 4;
    }
*/
}

CPsfVm::~CPsfVm()
{
    delete [] m_ram;
}

uint32 CPsfVm::LoadIopModule(const char* modulePath, uint32 baseAddress)
{
    const CPsfFs::FILE* file = m_fileSystem.GetFile(modulePath);
    CPtrStream stream(file->data, file->size);
    CELF elf(&stream);

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
