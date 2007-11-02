#ifndef _PSFVM_H_
#define _PSFVM_H_

#include "MIPS.h"
#include "PsfFs.h"

class CPsfVm
{
public:
                            CPsfVm(const char*);
    virtual                 ~CPsfVm();

    CMIPS&                  GetCpu();

private:
    typedef std::map<uint32, uint32> BlockMapType;

    enum RAMSIZE
    {
        RAMSIZE = 0x00400000,
    };

    enum DEFAULT_STACKSIZE
    {
        DEFAULT_STACKSIZE = 0x8000,
    };

    uint32                  LoadIopModule(const char*, uint32);
    uint32                  AllocateMemory(uint32);
    void                    FreeMemory(uint32);
    uint32                  Push(uint32&, const uint8*, uint32);
    static unsigned int     TickFunction(unsigned int);
    static void             SysCallHandler(CMIPS*);

    CMIPS                   m_cpu;
    CPsfFs                  m_fileSystem;
    uint8*                  m_ram;
    BlockMapType            m_blockMap;
};

#endif
