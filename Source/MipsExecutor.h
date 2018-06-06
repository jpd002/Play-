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
			assert(m_blockTable[address / INSTRUCTION_SIZE] == m_emptyBlock);
			m_blockTable[address / INSTRUCTION_SIZE] = block;
		}
	}

	void DeleteBlock(BlockType block)
	{
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += INSTRUCTION_SIZE)
		{
			assert(m_blockTable[address / INSTRUCTION_SIZE] != m_emptyBlock);
			m_blockTable[address / INSTRUCTION_SIZE] = m_emptyBlock;
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
				if(block == m_emptyBlock) continue;
				if(block == protectedBlock) continue;
				if(!IsInsideRange(block->GetBeginAddress(), start, end) && !IsInsideRange(block->GetEndAddress(), start, end)) continue;
				table[lo / INSTRUCTION_SIZE] = m_emptyBlock;
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
				for(uint32 i = 0; i < subTableSize; i++)
				{
					subTable[i] = m_emptyBlock;
				}
			}
			assert(subTable[loAddress / INSTRUCTION_SIZE] == m_emptyBlock);
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
			assert(subTable[loAddress / INSTRUCTION_SIZE] != m_emptyBlock);
			subTable[loAddress / INSTRUCTION_SIZE] = m_emptyBlock;
		}
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
	CMipsExecutor(CMIPS& context, uint32 maxAddress)
	: m_emptyBlock(std::make_shared<CBasicBlock>(context, MIPS_INVALID_PC, MIPS_INVALID_PC))
	, m_context(context)
	, m_maxAddress(maxAddress)
	, m_blockLookup(m_emptyBlock.get(), maxAddress)
	{
		m_emptyBlock->Compile();
		assert(!context.m_emptyBlockHandler);
		context.m_emptyBlockHandler = 
			[&] (CMIPS* context) 
			{
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
		while(m_context.m_State.cycleQuota > 0)
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
			if(m_context.m_State.nHasException) break;
		}
		return m_context.m_State.cycleQuota;
	}

	CBasicBlock* FindBlockStartingAt(uint32 address) const
	{
		auto result = m_blockLookup.FindBlockAt(address);
		if(result->GetBeginAddress() != address) return m_emptyBlock.get();
		return result;
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

	bool HasBlockAt(uint32 address) const
	{
		auto block = m_blockLookup.FindBlockAt(address);
		return !block->IsEmpty();
	}

	void CreateBlock(uint32 start, uint32 end)
	{
		{
			auto block = m_blockLookup.FindBlockAt(start);
			if(!block->IsEmpty())
			{
				//If the block starts and ends at the same place, block already exists and doesn't need
				//to be re-created
				uint32 otherBegin = block->GetBeginAddress();
				uint32 otherEnd = block->GetEndAddress();
				if((otherBegin == start) && (otherEnd == end))
				{
					return;
				}
				if(otherEnd == end)
				{
					//Repartition the existing block if end of both blocks are the same
					DeleteBlock(block);
					CreateBlock(otherBegin, start - 4);
					assert(!HasBlockAt(start));
				}
				else if(otherBegin == start)
				{
					DeleteBlock(block);
					CreateBlock(end + 4, otherEnd);
					assert(!HasBlockAt(end));
				}
				else
				{
					//Delete the currently existing block otherwise
					DeleteBlock(block);
				}
			}
		}
		assert(!HasBlockAt(end));
		{
			auto block = BlockFactory(m_context, start, end);
			m_blockLookup.AddBlock(block.get());
			m_blocks.push_back(std::move(block));
		}
	}

	virtual BasicBlockPtr BlockFactory(CMIPS& context, uint32 start, uint32 end)
	{
		auto result = std::make_shared<CBasicBlock>(context, start, end);
		result->Compile();
		return result;
	}

	virtual void PartitionFunction(uint32 functionAddress)
	{
		typedef std::set<uint32> PartitionPointSet;
		uint32 endAddress = 0;
		PartitionPointSet partitionPoints;

		//Insert begin point
		partitionPoints.insert(functionAddress);

		//Find the end
		for(uint32 address = functionAddress;; address += 4)
		{
			//Probably going too far...
			if((address - functionAddress) > 0x10000)
			{
				endAddress = address;
				partitionPoints.insert(endAddress);
				break;
			}
			uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
			if(opcode == 0x03E00008)
			{
				//+4 for delay slot
				endAddress = address + 4;
				partitionPoints.insert(endAddress + 4);
				break;
			}
		}

		//Find partition points within the function
		for(uint32 address = functionAddress; address <= endAddress; address += 4)
		{
			uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
			MIPS_BRANCH_TYPE branchType = m_context.m_pArch->IsInstructionBranch(&m_context, address, opcode);
			if(branchType == MIPS_BRANCH_NORMAL)
			{
				partitionPoints.insert(address + 8);
				uint32 target = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
				if(target > functionAddress && target < endAddress)
				{
					partitionPoints.insert(target);
				}
			}
			else if(branchType == MIPS_BRANCH_NODELAY)
			{
				partitionPoints.insert(address + 4);
			}
			//Check if there's a block already exising that this address
			if(address != endAddress)
			{
				auto possibleBlock = FindBlockStartingAt(address);
				if(!possibleBlock->IsEmpty())
				{
					//assert(possibleBlock->GetEndAddress() <= endAddress);
					//Add its beginning and end in the partition points
					partitionPoints.insert(possibleBlock->GetBeginAddress());
					partitionPoints.insert(possibleBlock->GetEndAddress() + 4);
				}
			}
		}

		//Check if blocks are too big
		{
			uint32 currentPoint = -1;
			for(PartitionPointSet::const_iterator pointIterator(partitionPoints.begin());
				pointIterator != partitionPoints.end(); ++pointIterator)
			{
				if(currentPoint != -1)
				{
					uint32 startPos = currentPoint;
					uint32 endPos = *pointIterator;
					uint32 distance = (endPos - startPos);
					if(distance > 0x400)
					{
						uint32 middlePos = ((endPos + startPos) / 2) & ~0x03;
						pointIterator = partitionPoints.insert(middlePos).first;
						--pointIterator;
						continue;
					}
				}
				currentPoint = *pointIterator;
			}
		}

		//Create blocks
		{
			uint32 currentPoint = -1;
			for(auto partitionPoint : partitionPoints)
			{
				if(currentPoint != -1)
				{
					CreateBlock(currentPoint, partitionPoint - 4);
				}
				currentPoint = partitionPoint;
			}
		}
	}

	void ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
	{
		auto clearedBlocks = m_blockLookup.ClearInRange(start, end, protectedBlock);
		if(!clearedBlocks.empty())
		{
			m_blocks.remove_if([&](const BasicBlockPtr& block) { return clearedBlocks.find(block.get()) != std::end(clearedBlocks); });
		}
	}

	BlockList m_blocks;
	BasicBlockPtr m_emptyBlock;
	CMIPS& m_context;
	uint32 m_maxAddress = 0;

	BlockLookupType m_blockLookup;

#ifdef DEBUGGER_INCLUDED
	bool m_breakpointsDisabledOnce = false;
#endif
};
