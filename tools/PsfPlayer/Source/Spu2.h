#ifndef _SPU2_H_
#define _SPU2_H_

#include <vector>
#include "Spu2_Core.h"

class CSpu2
{
public:
                CSpu2(uint32);
    virtual     ~CSpu2();

    uint32      ReadRegister(uint32);
    void        WriteRegister(uint32, uint32);

private:
    typedef std::vector<Spu2::CCore> CoreArrayType;

    void            LogRead(uint32);

    uint32          m_baseAddress;
    CoreArrayType   m_cores;
};

#endif
