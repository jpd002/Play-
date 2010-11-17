#ifndef _MIPSEXECUTOR_H_
#define _MIPSEXECUTOR_H_

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

class CMipsExecutor
{
public:
								CMipsExecutor(CMIPS&);
    virtual						~CMipsExecutor();
    int							Execute(int);
    bool						MustBreak() const;
    BasicBlockPtr				FindBlockAt(uint32) const;
    BasicBlockPtr				FindBlockStartingAt(uint32);
    virtual void				Reset();
	void						ClearActiveBlocks();

protected:
    typedef std::list<BasicBlockPtr> BlockList;
    typedef std::map<uint32, BasicBlockPtr, std::greater<uint32> > BlockBeginMap;
    typedef std::map<uint32, BasicBlockPtr> BlockEndMap;

    void						CreateBlock(uint32, uint32);
    void						DeleteBlock(const BasicBlockPtr&);
	virtual BasicBlockPtr		BlockFactory(CMIPS&, uint32, uint32);
    virtual void				PartitionFunction(uint32);

    BlockList					m_blocks;
    BlockBeginMap				m_blockBegin;
    BlockEndMap					m_blockEnd;
    CMIPS&						m_context;
};

#endif
