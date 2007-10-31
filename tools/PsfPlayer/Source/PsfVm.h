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
    enum RAMSIZE
    {
        RAMSIZE = 0x00400000,
    };

    uint32                  LoadIopModule(const char*, uint32);
    static unsigned int     TickFunction(unsigned int);

    CMIPS                   m_cpu;
    CPsfFs                  m_fileSystem;
    uint8*                  m_ram;
};

#endif
