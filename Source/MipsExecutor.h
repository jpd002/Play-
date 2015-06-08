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
	CBasicBlock*				FindBlockAt(uint32) const;
	CBasicBlock*				FindBlockStartingAt(uint32) const;
	void						DeleteBlock(CBasicBlock*);
	virtual void				Reset();
	void						ClearActiveBlocks();
	virtual void				ClearActiveBlocksInRange(uint32, uint32);

#ifdef DEBUGGER_INCLUDED
	bool						MustBreak() const;
	void						DisableBreakpointsOnce();
#endif

protected:
	typedef std::shared_ptr<CBasicBlock> BasicBlockPtr;
	typedef std::list<BasicBlockPtr> BlockList;

	void						CreateBlock(uint32, uint32);
	virtual BasicBlockPtr		BlockFactory(CMIPS&, uint32, uint32);
	virtual void				PartitionFunction(uint32);
	
	void						ClearActiveBlocksInRangeInternal(uint32, uint32, CBasicBlock*);

	BlockList					m_blocks;
	CMIPS&						m_context;

	CBasicBlock***				m_blockTable;
	uint32						m_subTableCount;

#ifdef DEBUGGER_INCLUDED
	bool						m_breakpointsDisabledOnce;
#endif
};

#endif
