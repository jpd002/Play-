#pragma once

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

static bool IsInsideRange(uint32 address, uint32 start, uint32 end)
{
	return (address >= start) && (address <= end);
}

class BlockLookupOneWay
{
public:
	typedef CBasicBlock* BlockType;
	
	BlockLookupOneWay(uint32 maxAddress)
	{
		m_tableSize = maxAddress / INSTRUCTION_SIZE;
		m_blockTable = new BlockType[m_tableSize];
		memset(m_blockTable, 0, sizeof(BlockType) * m_tableSize);
	}
	
	~BlockLookupOneWay()
	{
		delete[] m_blockTable;
	}
	
	void Clear()
	{
		for(unsigned int i = 0; i < m_tableSize; i++)
		{
			m_blockTable[i] = nullptr;
		}
	}
	
	std::set<BlockType> ClearInRange(uint32 start, uint32 end, BlockType protectedBlock)
	{
		std::set<BlockType> clearedBlocks;

		for(uint32 address = start; address <= end; address += INSTRUCTION_SIZE)
		{
			auto block = m_blockTable[address / INSTRUCTION_SIZE];
			if(block == nullptr) continue;
			if(block == protectedBlock) continue;
			if(!IsInsideRange(block->GetBeginAddress(), start, end) && !IsInsideRange(block->GetEndAddress(), start, end)) continue;
			DeleteBlock(block);
			clearedBlocks.insert(block);
		}
		
		return clearedBlocks;
	}
	
	void AddBlock(BlockType block)
	{
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += INSTRUCTION_SIZE)
		{
			assert(!m_blockTable[address / INSTRUCTION_SIZE]);
			m_blockTable[address / INSTRUCTION_SIZE] = block;
		}
	}
	
	void DeleteBlock(BlockType block)
	{
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += INSTRUCTION_SIZE)
		{
			assert(m_blockTable[address / INSTRUCTION_SIZE]);
			m_blockTable[address / INSTRUCTION_SIZE] = nullptr;
		}
	}
	
	BlockType FindBlockAt(uint32 address) const
	{
		assert((address / INSTRUCTION_SIZE) < m_tableSize);
		return m_blockTable[address / INSTRUCTION_SIZE];
	}
	
private:
	enum
	{
		INSTRUCTION_SIZE = 4,
	};

	BlockType* m_blockTable = nullptr;
	uint32 m_tableSize = 0;
};

class BlockLookupTwoWay
{
public:
	typedef CBasicBlock* BlockType;

	BlockLookupTwoWay(uint32 maxAddress)
	{
		m_subTableCount = (maxAddress + SUBTABLE_MASK) / SUBTABLE_SIZE;
		assert(m_subTableCount != 0);
		m_blockTable = new BlockType*[m_subTableCount];
		memset(m_blockTable, 0, sizeof(BlockType*) * m_subTableCount);
	}
	
	~BlockLookupTwoWay()
	{
		for(unsigned int i = 0; i < m_subTableCount; i++)
		{
			auto subTable = m_blockTable[i];
			if(subTable)
			{
				delete[] subTable;
			}
		}
		delete[] m_blockTable;
	}
	
	void Clear()
	{
		for(unsigned int i = 0; i < m_subTableCount; i++)
		{
			auto subTable = m_blockTable[i];
			if(subTable)
			{
				delete[] subTable;
				m_blockTable[i] = nullptr;
			}
		}
	}
	
	std::set<BlockType> ClearInRange(uint32 start, uint32 end, BlockType protectedBlock)
	{
		uint32 hiStart = start >> SUBTABLE_BITS;
		uint32 hiEnd = end >> SUBTABLE_BITS;

		//TODO: improve this
		//We need to increase the range to make sure we catch any
		//block that are straddling table boundaries
		if(hiStart != 0) hiStart--;
		if(hiEnd != (m_subTableCount - 1)) hiEnd++;

		std::set<BlockType> clearedBlocks;

		for(uint32 hi = hiStart; hi <= hiEnd; hi++)
		{
			auto table = m_blockTable[hi];
			if(!table) continue;

			for(uint32 lo = 0; lo < SUBTABLE_SIZE; lo += INSTRUCTION_SIZE)
			{
				uint32 tableAddress = (hi << SUBTABLE_BITS) | lo;
				auto block = table[lo / INSTRUCTION_SIZE];
				if(block == nullptr) continue;
				if(block == protectedBlock) continue;
				if(!IsInsideRange(block->GetBeginAddress(), start, end) && !IsInsideRange(block->GetEndAddress(), start, end)) continue;
				table[lo / INSTRUCTION_SIZE] = nullptr;
				clearedBlocks.insert(block);
			}
		}
		
		return clearedBlocks;
	}
	
	void AddBlock(BlockType block)
	{
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += INSTRUCTION_SIZE)
		{
			uint32 hiAddress = address >> SUBTABLE_BITS;
			uint32 loAddress = address & SUBTABLE_MASK;
			assert(hiAddress < m_subTableCount);
			auto& subTable = m_blockTable[hiAddress];
			if(!subTable)
			{
				const uint32 subTableSize = SUBTABLE_SIZE / INSTRUCTION_SIZE;
				subTable = new BlockType[subTableSize];
				memset(subTable, 0, sizeof(BlockType) * subTableSize);
			}
			assert(!subTable[loAddress / INSTRUCTION_SIZE]);
			subTable[loAddress / INSTRUCTION_SIZE] = block;
		}
	}
	
	void DeleteBlock(BlockType block)
	{
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += INSTRUCTION_SIZE)
		{
			uint32 hiAddress = address >> SUBTABLE_BITS;
			uint32 loAddress = address & SUBTABLE_MASK;
			assert(hiAddress < m_subTableCount);
			auto& subTable = m_blockTable[hiAddress];
			assert(subTable);
			assert(subTable[loAddress / INSTRUCTION_SIZE]);
			subTable[loAddress / INSTRUCTION_SIZE] = nullptr;
		}
	}
	
	BlockType FindBlockAt(uint32 address) const
	{
		uint32 hiAddress = address >> SUBTABLE_BITS;
		uint32 loAddress = address & SUBTABLE_MASK;
		assert(hiAddress < m_subTableCount);
		auto& subTable = m_blockTable[hiAddress];
		if(!subTable) return nullptr;
		auto result = subTable[loAddress / INSTRUCTION_SIZE];
		return result;
	}
	
private:
	enum
	{
		SUBTABLE_BITS = 16,
		SUBTABLE_SIZE = (1 << SUBTABLE_BITS),
		SUBTABLE_MASK = (SUBTABLE_SIZE - 1),
		INSTRUCTION_SIZE = 4,
	};
	
	BlockType** m_blockTable = nullptr;
	uint32 m_subTableCount = 0;
};

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

	BlockLookupTwoWay m_blockLookup;
	
#ifdef DEBUGGER_INCLUDED
	bool m_breakpointsDisabledOnce = false;
#endif
};
