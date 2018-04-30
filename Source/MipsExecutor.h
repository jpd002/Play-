#pragma once

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

class CMipsExecutor
{
public:
	CMipsExecutor(CMIPS&, uint32);
	virtual ~CMipsExecutor();

	template <uint32 (*TranslateFunction)(CMIPS*, uint32) = CMIPS::TranslateAddress64>
	int Execute(int cycles)
	{
		assert(TranslateFunction == m_context.m_pAddrTranslator);
		CBasicBlock* block(nullptr);
		while(cycles > 0)
		{
			uint32 address = TranslateFunction(&m_context, m_context.m_State.nPC);
			if(!block || address != block->GetBeginAddress())
			{
				block = FindBlockStartingAt(address);
				if(!block)
				{
					//We need to partition the space and compile the blocks
					PartitionFunction(address);
					block = FindBlockStartingAt(address);
					assert(block);
				}
			}

#ifdef DEBUGGER_INCLUDED
			if(!m_breakpointsDisabledOnce && MustBreak()) break;
			m_breakpointsDisabledOnce = false;
#endif
			cycles -= block->Execute();
			if(m_context.m_State.nHasException) break;
		}
		return cycles;
	}

	CBasicBlock* FindBlockAt(uint32) const;
	CBasicBlock* FindBlockStartingAt(uint32) const;
	void DeleteBlock(CBasicBlock*);
	virtual void Reset();
	void ClearActiveBlocks();
	virtual void ClearActiveBlocksInRange(uint32, uint32);

#ifdef DEBUGGER_INCLUDED
	bool MustBreak() const;
	void DisableBreakpointsOnce();
#endif

protected:
	typedef std::shared_ptr<CBasicBlock> BasicBlockPtr;
	typedef std::list<BasicBlockPtr> BlockList;

	void CreateBlock(uint32, uint32);
	virtual BasicBlockPtr BlockFactory(CMIPS&, uint32, uint32);
	virtual void PartitionFunction(uint32);

	void ClearActiveBlocksInRangeInternal(uint32, uint32, CBasicBlock*);

	BlockList m_blocks;
	CMIPS& m_context;
	uint32 m_maxAddress = 0;

	CBasicBlock*** m_blockTable = nullptr;
	uint32 m_subTableCount = 0;

#ifdef DEBUGGER_INCLUDED
	bool m_breakpointsDisabledOnce = false;
#endif
};
