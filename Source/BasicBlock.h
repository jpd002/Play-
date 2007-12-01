#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "MIPS.h"

class CBasicBlock
{
public:
                CBasicBlock(CMIPS&, uint32, uint32);
    virtual     ~CBasicBlock();
    void        Execute();
    void        Compile();

private:

    uint8*      m_text;
    uint32      m_begin;
    uint32      m_end;
    CMIPS&      m_context;
};

#endif
