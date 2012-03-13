#ifndef _MIPSEXECUTOR_H_
#define _MIPSEXECUTOR_H_

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

class CMipsExecutor
{
public:
								CMipsExecutor(CMIPS&, uint32);
	virtual						~CMipsExecutor();
	int							Execute(int);
	bool						MustBreak() const;
	CBasicBlock*				FindBlockAt(uint32) const;
	CBasicBlock*				FindBlockStartingAt(uint32) const;
	void						DeleteBlock(CBasicBlock*);
	virtual void				Reset();
	void						ClearActiveBlocks();

protected:
	typedef std::list<BasicBlockPtr> BlockList;

	void						CreateBlock(uint32, uint32);
	virtual BasicBlockPtr		BlockFactory(CMIPS&, uint32, uint32);
	virtual void				PartitionFunction(uint32);

	BlockList					m_blocks;
	CMIPS&						m_context;

	CBasicBlock***				m_blockTable;
	uint32						m_subTableCount;
};

#endif
