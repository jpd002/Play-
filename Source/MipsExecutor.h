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

	BlockLookupOneWay(BlockType emptyBlock, uint32 maxAddress)
	    : m_emptyBlock(emptyBlock)
	{
		m_tableSize = maxAddress / INSTRUCTION_SIZE;
		m_blockTable = new BlockType[m_tableSize];
	}

	~BlockLookupOneWay()
	{
		delete[] m_blockTable;
	}

	void Clear()
	{
		for(unsigned int i = 0; i < m_tableSize; i++)
		{
			m_blockTable[i] = m_emptyBlock;
		}
	}

	void AddBlock(BlockType block)
	{
		uint32 address = block->GetBeginAddress();
		assert(m_blockTable[address / INSTRUCTION_SIZE] == m_emptyBlock);
		m_blockTable[address / INSTRUCTION_SIZE] = block;
	}

	void DeleteBlock(BlockType block)
	{
		uint32 address = block->GetBeginAddress();
		assert(m_blockTable[address / INSTRUCTION_SIZE] != m_emptyBlock);
		m_blockTable[address / INSTRUCTION_SIZE] = m_emptyBlock;
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

	BlockType m_emptyBlock = nullptr;
	BlockType* m_blockTable = nullptr;
	uint32 m_tableSize = 0;
};

class BlockLookupTwoWay
{
public:
	typedef CBasicBlock* BlockType;

	BlockLookupTwoWay(BlockType emptyBlock, uint32 maxAddress)
	    : m_emptyBlock(emptyBlock)
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

	void AddBlock(BlockType block)
	{
		uint32 address = block->GetBeginAddress();
		uint32 hiAddress = address >> SUBTABLE_BITS;
		uint32 loAddress = address & SUBTABLE_MASK;
		assert(hiAddress < m_subTableCount);
		auto& subTable = m_blockTable[hiAddress];
		if(!subTable)
		{
			const uint32 subTableSize = SUBTABLE_SIZE / INSTRUCTION_SIZE;
			subTable = new BlockType[subTableSize];
			for(uint32 i = 0; i < subTableSize; i++)
			{
				subTable[i] = m_emptyBlock;
			}
		}
		assert(subTable[loAddress / INSTRUCTION_SIZE] == m_emptyBlock);
		subTable[loAddress / INSTRUCTION_SIZE] = block;
	}

	void DeleteBlock(BlockType block)
	{
		uint32 address = block->GetBeginAddress();
		uint32 hiAddress = address >> SUBTABLE_BITS;
		uint32 loAddress = address & SUBTABLE_MASK;
		assert(hiAddress < m_subTableCount);
		auto& subTable = m_blockTable[hiAddress];
		assert(subTable);
		assert(subTable[loAddress / INSTRUCTION_SIZE] != m_emptyBlock);
		subTable[loAddress / INSTRUCTION_SIZE] = m_emptyBlock;
	}

	BlockType FindBlockAt(uint32 address) const
	{
		uint32 hiAddress = address >> SUBTABLE_BITS;
		uint32 loAddress = address & SUBTABLE_MASK;
		assert(hiAddress < m_subTableCount);
		auto& subTable = m_blockTable[hiAddress];
		if(!subTable) return m_emptyBlock;
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

	BlockType m_emptyBlock = nullptr;
	BlockType** m_blockTable = nullptr;
	uint32 m_subTableCount = 0;
};

typedef uint32 (*TranslateFunctionType)(CMIPS*, uint32);
typedef std::shared_ptr<CBasicBlock> BasicBlockPtr;

template <typename BlockLookupType, TranslateFunctionType TranslateFunction = &CMIPS::TranslateAddress64>
class CMipsExecutor
{
public:
	enum
	{
		MAX_BLOCK_SIZE = 0x1000,
	};

	CMipsExecutor(CMIPS& context, uint32 maxAddress)
	    : m_emptyBlock(std::make_shared<CBasicBlock>(context, MIPS_INVALID_PC, MIPS_INVALID_PC))
	    , m_context(context)
	    , m_maxAddress(maxAddress)
	    , m_blockLookup(m_emptyBlock.get(), maxAddress)
	{
		m_emptyBlock->Compile();
		assert(!context.m_emptyBlockHandler);
		context.m_emptyBlockHandler =
		    [&](CMIPS* context) {
			    uint32 address = context->m_pAddrTranslator(&m_context, m_context.m_State.nPC);
			    PartitionFunction(address);
			    auto block = FindBlockStartingAt(address);
			    assert(!block->IsEmpty());
			    block->Execute();
		    };
	}

	virtual ~CMipsExecutor() = default;

	int Execute(int cycles)
	{
		assert(TranslateFunction == m_context.m_pAddrTranslator);
		auto block = m_emptyBlock.get();
		m_context.m_State.cycleQuota = cycles;
		while(m_context.m_State.nHasException == 0)
		{
			uint32 address = TranslateFunction(&m_context, m_context.m_State.nPC);
			if(address != block->GetBeginAddress())
			{
				block = FindBlockStartingAt(address);
			}

#ifdef DEBUGGER_INCLUDED
			if(!m_breakpointsDisabledOnce && MustBreak()) break;
			m_breakpointsDisabledOnce = false;
#endif
			block->Execute();
		}
		m_context.m_State.nHasException &= ~MIPS_EXECUTION_STATUS_QUOTADONE;
		return m_context.m_State.cycleQuota;
	}

	CBasicBlock* FindBlockStartingAt(uint32 address) const
	{
		return m_blockLookup.FindBlockAt(address);
	}

	void DeleteBlock(CBasicBlock* block)
	{
		assert(block != m_emptyBlock.get());

		m_blockLookup.DeleteBlock(block);

		//Remove block from our lists
		auto blockIterator = std::find_if(std::begin(m_blocks), std::end(m_blocks), [&](const BasicBlockPtr& blockPtr) { return blockPtr.get() == block; });
		assert(blockIterator != std::end(m_blocks));
		m_blocks.erase(blockIterator);
	}

	virtual void Reset()
	{
		ClearActiveBlocks();
	}

	void ClearActiveBlocks()
	{
		m_blockLookup.Clear();
		m_blocks.clear();
	}

	virtual void ClearActiveBlocksInRange(uint32 start, uint32 end)
	{
		ClearActiveBlocksInRangeInternal(start, end, nullptr);
	}

#ifdef DEBUGGER_INCLUDED
	bool MustBreak() const
	{
		uint32 currentPc = m_context.m_pAddrTranslator(&m_context, m_context.m_State.nPC);
		auto block = m_blockLookup.FindBlockAt(currentPc);
		for(auto breakPointAddress : m_context.m_breakpoints)
		{
			if(currentPc == breakPointAddress) return true;
			if(block != NULL)
			{
				if(breakPointAddress >= block->GetBeginAddress() && breakPointAddress <= block->GetEndAddress()) return true;
			}
		}
		return false;
	}

	void DisableBreakpointsOnce()
	{
		m_breakpointsDisabledOnce = true;
	}
#endif

protected:
	typedef std::list<BasicBlockPtr> BlockList;
	typedef std::multimap<uint32, uint32> BlockLinkMap;

	bool HasBlockAt(uint32 address) const
	{
		auto block = m_blockLookup.FindBlockAt(address);
		return !block->IsEmpty();
	}

	void CreateBlock(uint32 start, uint32 end)
	{
		assert(!HasBlockAt(start));
		auto block = BlockFactory(m_context, start, end);
		m_blockLookup.AddBlock(block.get());
		m_blocks.push_back(std::move(block));
	}

	virtual BasicBlockPtr BlockFactory(CMIPS& context, uint32 start, uint32 end)
	{
		auto result = std::make_shared<CBasicBlock>(context, start, end);
		result->Compile();
		return result;
	}

	void SetupBlockLinks(uint32 startAddress, uint32 endAddress, uint32 branchAddress)
	{
		auto block = m_blockLookup.FindBlockAt(startAddress);

		{
			uint32 nextBlockAddress = endAddress + 4;
			auto nextBlock = m_blockLookup.FindBlockAt(nextBlockAddress);
			if(!nextBlock->IsEmpty())
			{
				block->LinkNextBlock(nextBlock);
			}
			else
			{
				m_pendingNextBlockLinks.insert(std::make_pair(nextBlockAddress, startAddress));
			}
		}

		if(branchAddress != 0)
		{
			auto branchBlock = m_blockLookup.FindBlockAt(branchAddress);
			if(!branchBlock->IsEmpty())
			{
				block->LinkBranchBlock(branchBlock);
			}
			else
			{
				m_pendingBranchBlockLinks.insert(std::make_pair(branchAddress, startAddress));
			}
		}

		//Resolve any block links that could be valid now that block has been created
		{
			auto lowerBound = m_pendingNextBlockLinks.lower_bound(startAddress);
			auto upperBound = m_pendingNextBlockLinks.upper_bound(startAddress);
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				auto referringBlock = m_blockLookup.FindBlockAt(blockLinkIterator->second);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->LinkNextBlock(block);
			}
			m_pendingNextBlockLinks.erase(lowerBound, upperBound);
		}
		{
			auto lowerBound = m_pendingBranchBlockLinks.lower_bound(startAddress);
			auto upperBound = m_pendingBranchBlockLinks.upper_bound(startAddress);
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				auto referringBlock = m_blockLookup.FindBlockAt(blockLinkIterator->second);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->LinkBranchBlock(block);
			}
			m_pendingBranchBlockLinks.erase(lowerBound, upperBound);
		}
	}

	virtual void PartitionFunction(uint32 startAddress)
	{
		uint32 endAddress = startAddress + MAX_BLOCK_SIZE;
		uint32 branchAddress = 0;
		for(uint32 address = startAddress; address < endAddress; address += 4)
		{
			uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
			auto branchType = m_context.m_pArch->IsInstructionBranch(&m_context, address, opcode);
			if(branchType == MIPS_BRANCH_NORMAL)
			{
				branchAddress = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
				endAddress = address + 4;
				break;
			}
			else if(branchType == MIPS_BRANCH_NODELAY)
			{
				endAddress = address;
				break;
			}
		}
		assert((endAddress - startAddress) <= MAX_BLOCK_SIZE);
		CreateBlock(startAddress, endAddress);
		SetupBlockLinks(startAddress, endAddress, branchAddress);
	}

	void ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
	{
		//Widen scan range since blocks starting before the range can end in the range
		uint32 scanStart = static_cast<uint32>(std::max<int64>(0, static_cast<uint64>(start) - MAX_BLOCK_SIZE));
		uint32 scanEnd = end;
		assert(scanEnd > scanStart);

		std::set<CBasicBlock*> clearedBlocks;
		for(uint32 address = scanStart; address <= scanEnd; address += 4)
		{
			auto block = m_blockLookup.FindBlockAt(address);
			if(block->IsEmpty()) continue;
			if(block == protectedBlock) continue;
			if(!IsInsideRange(block->GetBeginAddress(), start, end) && !IsInsideRange(block->GetEndAddress(), start, end)) continue;
			clearedBlocks.insert(block);
			m_blockLookup.DeleteBlock(block);
		}

		if(!clearedBlocks.empty())
		{
			m_blocks.remove_if([&](const BasicBlockPtr& block) { return clearedBlocks.find(block.get()) != std::end(clearedBlocks); });
		}
	}

	BlockList m_blocks;
	BasicBlockPtr m_emptyBlock;
	BlockLinkMap m_pendingNextBlockLinks;
	BlockLinkMap m_pendingBranchBlockLinks;
	CMIPS& m_context;
	uint32 m_maxAddress = 0;

	BlockLookupType m_blockLookup;

#ifdef DEBUGGER_INCLUDED
	bool m_breakpointsDisabledOnce = false;
#endif
};
