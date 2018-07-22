#pragma once

#include <list>
#include "MIPS.h"
#include "BasicBlock.h"

#include "BlockLookupOneWay.h"
#include "BlockLookupTwoWay.h"

static bool IsInsideRange(uint32 address, uint32 start, uint32 end)
{
	return (address >= start) && (address <= end);
}

typedef uint32 (*TranslateFunctionType)(CMIPS*, uint32);
typedef std::shared_ptr<CBasicBlock> BasicBlockPtr;

template <typename BlockLookupType>
class CGenericMipsExecutor : public CMipsExecutor
{
public:
	enum
	{
		MAX_BLOCK_SIZE = 0x1000,
	};

	CGenericMipsExecutor(CMIPS& context, uint32 maxAddress)
	    : m_emptyBlock(std::make_shared<CBasicBlock>(context, MIPS_INVALID_PC, MIPS_INVALID_PC))
	    , m_context(context)
	    , m_maxAddress(maxAddress)
	    , m_addressMask(maxAddress - 1)
	    , m_blockLookup(m_emptyBlock.get(), maxAddress)
	{
		m_emptyBlock->Compile();
		assert(!context.m_emptyBlockHandler);
		context.m_emptyBlockHandler =
		    [&](CMIPS* context) {
			    uint32 address = m_context.m_State.nPC & m_addressMask;
			    PartitionFunction(address);
			    auto block = FindBlockStartingAt(address);
			    assert(!block->IsEmpty());
			    block->Execute();
		    };
	}

	virtual ~CGenericMipsExecutor() = default;

	int Execute(int cycles) override
	{
		m_context.m_State.cycleQuota = cycles;
#ifdef DEBUGGER_INCLUDED
		m_mustBreak = false;
		m_initQuota = cycles;
#endif
		while(m_context.m_State.nHasException == 0)
		{
			uint32 address = m_context.m_State.nPC & m_addressMask;
			auto block = m_blockLookup.FindBlockAt(address);
			block->Execute();
		}
		m_context.m_State.nHasException &= ~MIPS_EXECUTION_STATUS_QUOTADONE;
#ifdef DEBUGGER_INCLUDED
		if(m_context.m_State.nHasException == MIPS_EXCEPTION_BREAKPOINT)
		{
			m_mustBreak = true;
			m_context.m_State.nHasException = MIPS_EXCEPTION_NONE;
		}
		m_breakpointsDisabledOnce = false;
#endif
		return m_context.m_State.cycleQuota;
	}

	CBasicBlock* FindBlockStartingAt(uint32 address) const
	{
		return m_blockLookup.FindBlockAt(address);
	}

	void Reset() override
	{
		m_blockLookup.Clear();
		m_blocks.clear();
		m_blockLinks.clear();
		m_pendingBlockLinks.clear();
	}

	void ClearActiveBlocksInRange(uint32 start, uint32 end, bool executing) override
	{
		CBasicBlock* currentBlock = nullptr;
		if(executing)
		{
			currentBlock = FindBlockStartingAt(m_context.m_State.nPC);
			assert(!currentBlock->IsEmpty());
		}
		ClearActiveBlocksInRangeInternal(start, end, currentBlock);
	}

#ifdef DEBUGGER_INCLUDED
	bool MustBreak() const override
	{
		return m_mustBreak;
	}

	void DisableBreakpointsOnce() override
	{
		m_breakpointsDisabledOnce = true;
	}

	bool FilterBreakpoint() override
	{
		return ((m_initQuota == m_context.m_State.cycleQuota) && m_breakpointsDisabledOnce);
	}
#endif

protected:
	//Outgoing block link
	struct BLOCK_LINK
	{
		CBasicBlock::LINK_SLOT slot;
		uint32 address;
	};

	typedef std::list<BasicBlockPtr> BlockList;
	typedef std::multimap<uint32, BLOCK_LINK> BlockLinkMap;

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
			uint32 nextBlockAddress = (endAddress + 4) & m_addressMask;
			block->SetLinkTargetAddress(CBasicBlock::LINK_SLOT_NEXT, nextBlockAddress);
			auto link = std::make_pair(nextBlockAddress, BLOCK_LINK{CBasicBlock::LINK_SLOT_NEXT, startAddress});
			auto nextBlock = m_blockLookup.FindBlockAt(nextBlockAddress);
			if(!nextBlock->IsEmpty())
			{
				block->LinkBlock(CBasicBlock::LINK_SLOT_NEXT, nextBlock);
				m_blockLinks.insert(link);
			}
			else
			{
				m_pendingBlockLinks.insert(link);
			}
		}

		if(branchAddress != 0)
		{
			branchAddress &= m_addressMask;
			block->SetLinkTargetAddress(CBasicBlock::LINK_SLOT_BRANCH, branchAddress);
			auto link = std::make_pair(branchAddress, BLOCK_LINK{CBasicBlock::LINK_SLOT_BRANCH, startAddress});
			auto branchBlock = m_blockLookup.FindBlockAt(branchAddress);
			if(!branchBlock->IsEmpty())
			{
				block->LinkBlock(CBasicBlock::LINK_SLOT_BRANCH, branchBlock);
				m_blockLinks.insert(link);
			}
			else
			{
				m_pendingBlockLinks.insert(link);
			}
		}

		//Resolve any block links that could be valid now that block has been created
		{
			auto lowerBound = m_pendingBlockLinks.lower_bound(startAddress);
			auto upperBound = m_pendingBlockLinks.upper_bound(startAddress);
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				const auto& blockLink = blockLinkIterator->second;
				auto referringBlock = m_blockLookup.FindBlockAt(blockLink.address);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->LinkBlock(blockLink.slot, block);
				m_blockLinks.insert(*blockLinkIterator);
			}
			m_pendingBlockLinks.erase(lowerBound, upperBound);
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
		assert(endAddress <= m_maxAddress);
		CreateBlock(startAddress, endAddress);
		SetupBlockLinks(startAddress, endAddress, branchAddress);
	}

	//Unlink and removes block from all of our bookkeeping structures
	void OrphanBlock(CBasicBlock* block)
	{
		auto orphanBlockLinkSlot =
		    [&](CBasicBlock::LINK_SLOT linkSlot) {
			    auto slotSearch =
			        [&](const std::pair<uint32, BLOCK_LINK>& link) {
				        return (link.second.address == block->GetBeginAddress()) &&
				               (link.second.slot == linkSlot);
			        };
			    uint32 linkTargetAddress = block->GetLinkTargetAddress(linkSlot);
			    //Check if block has this specific link slot
			    if(linkTargetAddress != MIPS_INVALID_PC)
			    {
				    //If it has that link slot, it's either linked or pending to be linked
				    auto slotIterator = std::find_if(m_blockLinks.begin(), m_blockLinks.end(), slotSearch);
				    if(slotIterator != std::end(m_blockLinks))
				    {
					    block->UnlinkBlock(linkSlot);
					    m_blockLinks.erase(slotIterator);
				    }
				    else
				    {
					    slotIterator = std::find_if(m_pendingBlockLinks.begin(), m_pendingBlockLinks.end(), slotSearch);
					    assert(slotIterator != std::end(m_pendingBlockLinks));
					    m_pendingBlockLinks.erase(slotIterator);
				    }
			    }
		    };
		orphanBlockLinkSlot(CBasicBlock::LINK_SLOT_NEXT);
		orphanBlockLinkSlot(CBasicBlock::LINK_SLOT_BRANCH);
	}

	void ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
	{
		//Widen scan range since blocks starting before the range can end in the range
		uint32 scanStart = static_cast<uint32>(std::max<int64>(0, static_cast<uint64>(start) - MAX_BLOCK_SIZE));
		uint32 scanEnd = end;
		assert(scanEnd > scanStart);

		std::set<CBasicBlock*> clearedBlocks;
		for(uint32 address = scanStart; address < scanEnd; address += 4)
		{
			auto block = m_blockLookup.FindBlockAt(address);
			if(block->IsEmpty()) continue;
			if(block == protectedBlock) continue;
			if(!IsInsideRange(block->GetBeginAddress(), start, end) && !IsInsideRange(block->GetEndAddress(), start, end)) continue;
			clearedBlocks.insert(block);
			m_blockLookup.DeleteBlock(block);
		}

		//Remove pending block link entries for the blocks that are about to be cleared
		for(auto& block : clearedBlocks)
		{
			OrphanBlock(block);
		}

		//Undo all stale links
		for(auto& block : clearedBlocks)
		{
			auto lowerBound = m_blockLinks.lower_bound(block->GetBeginAddress());
			auto upperBound = m_blockLinks.upper_bound(block->GetBeginAddress());
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				const auto& blockLink = blockLinkIterator->second;
				auto referringBlock = m_blockLookup.FindBlockAt(blockLink.address);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->UnlinkBlock(blockLink.slot);
				m_pendingBlockLinks.insert(*blockLinkIterator);
			}
			m_blockLinks.erase(lowerBound, upperBound);
		}

		if(!clearedBlocks.empty())
		{
			m_blocks.remove_if([&](const BasicBlockPtr& block) { return clearedBlocks.find(block.get()) != std::end(clearedBlocks); });
		}
	}

	BlockList m_blocks;
	BasicBlockPtr m_emptyBlock;
	BlockLinkMap m_blockLinks;
	BlockLinkMap m_pendingBlockLinks;
	CMIPS& m_context;
	uint32 m_maxAddress = 0;
	uint32 m_addressMask = 0;

	BlockLookupType m_blockLookup;

#ifdef DEBUGGER_INCLUDED
	bool m_mustBreak = false;
	bool m_breakpointsDisabledOnce = false;
	int m_initQuota = 0;
#endif
};
