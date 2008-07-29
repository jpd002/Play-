#ifndef _MIPSEXECUTOR_H_
#define _MIPSEXECUTOR_H_

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

class CMipsExecutor
{
public:
                    CMipsExecutor(CMIPS&);
    virtual         ~CMipsExecutor();
    int             Execute(int);
    bool            MustBreak();
    CBasicBlock*    FindBlockAt(uint32);
    CBasicBlock*    FindBlockStartingAt(uint32);
    void            Clear();

protected:
    typedef std::list<CBasicBlock*> BlockList;
    typedef std::map<uint32, CBasicBlock*, std::greater<uint32> > BlockBeginMap;
    typedef std::map<uint32, CBasicBlock*> BlockEndMap;

    void            CreateBlock(uint32, uint32);
    void            DeleteBlock(CBasicBlock*);
    virtual void    PartitionFunction(uint32);

    BlockList       m_blocks;
    BlockBeginMap   m_blockBegin;
    BlockEndMap     m_blockEnd;
    CMIPS&          m_context;
};

#endif
