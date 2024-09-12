#pragma once

#include <unordered_set>
#include "MIPS.h"
#include "BasicBlock.h"

#include "BlockLookupOneWay.h"
#include "BlockLookupTwoWay.h"

FRAMEWORK_MAYBE_UNUSED
static bool RangesOverlap(uint32 x1, uint32 x2, uint32 y1, uint32 y2)
{
	assert(x1 <= x2);
	assert(y1 <= y2);
	return (x1 <= y2) && (y1 <= x2);
}

typedef uint32 (*TranslateFunctionType)(CMIPS*, uint32);
typedef std::shared_ptr<CBasicBlock> BasicBlockPtr;

template <typename BlockLookupType, uint32 instructionSize = 4>
class CGenericMipsExecutor : public CMipsExecutor
{
public:
	enum
	{
		MAX_BLOCK_SIZE = 0x1000,
	};

	enum
	{
		RECYCLE_NOLINK_THRESHOLD = 16,
	};

	CGenericMipsExecutor(CMIPS& context, uint32 maxAddress, BLOCK_CATEGORY blockCategory)
	    : m_emptyBlock(std::make_shared<CBasicBlock>(context, MIPS_INVALID_PC, MIPS_INVALID_PC, blockCategory))
	    , m_context(context)
	    , m_maxAddress(maxAddress)
	    , m_addressMask(maxAddress - 1)
	    , m_blockCategory(blockCategory)
	    , m_blockLookup(m_emptyBlock.get(), maxAddress)
	{
		m_emptyBlock->Compile();
		ResetBlockOutLinks(m_emptyBlock.get());

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
		m_context.m_State.nHasException &= ~MIPS_EXCEPTION_STATUS_QUOTADONE;
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
		m_blockOutLinks.clear();
#ifdef DEBUGGER_INCLUDED
		m_mustBreak = false;
#endif
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
	typedef std::unordered_set<BasicBlockPtr> BlockStore;

	bool HasBlockAt(uint32 address) const
	{
		auto block = m_blockLookup.FindBlockAt(address);
		return !block->IsEmpty();
	}

	void CreateBlock(uint32 start, uint32 end)
	{
		assert(!HasBlockAt(start));
		auto block = BlockFactory(m_context, start, end);
		ResetBlockOutLinks(block.get());
		m_blockLookup.AddBlock(block.get());
		m_blocks.insert(std::move(block));
	}

	void ResetBlockOutLinks(CBasicBlock* block)
	{
		for(uint32 i = 0; i < LINK_SLOT_MAX; i++)
		{
			block->SetOutLink(static_cast<LINK_SLOT>(i), std::end(m_blockOutLinks));
		}
	}

	virtual BasicBlockPtr BlockFactory(CMIPS& context, uint32 start, uint32 end)
	{
		auto result = std::make_shared<CBasicBlock>(context, start, end, m_blockCategory);
		result->Compile();
		return result;
	}

	void SetupBlockLinks(uint32 startAddress, uint32 endAddress, uint32 branchAddress)
	{
		auto block = m_blockLookup.FindBlockAt(startAddress);

		{
			uint32 nextBlockAddress = (endAddress + 4) & m_addressMask;
			const auto linkSlot = LINK_SLOT_NEXT;
			auto link = m_blockOutLinks.insert(std::make_pair(nextBlockAddress, BLOCK_OUT_LINK{linkSlot, startAddress, false}));
			block->SetOutLink(linkSlot, link);

			auto nextBlock = m_blockLookup.FindBlockAt(nextBlockAddress);
			if(!nextBlock->IsEmpty())
			{
				block->LinkBlock(linkSlot, nextBlock);
				link->second.live = true;
			}
		}

		if((branchAddress != MIPS_INVALID_PC) && block->HasLinkSlot(LINK_SLOT_BRANCH))
		{
			branchAddress &= m_addressMask;
			const auto linkSlot = LINK_SLOT_BRANCH;
			auto link = m_blockOutLinks.insert(std::make_pair(branchAddress, BLOCK_OUT_LINK{linkSlot, startAddress, false}));
			block->SetOutLink(linkSlot, link);

			auto branchBlock = m_blockLookup.FindBlockAt(branchAddress);
			if(!branchBlock->IsEmpty())
			{
				block->LinkBlock(linkSlot, branchBlock);
				link->second.live = true;
			}
		}
		else
		{
			block->SetOutLink(LINK_SLOT_BRANCH, std::end(m_blockOutLinks));
		}

		//Resolve any block links that could be valid now that block has been created
		{
			auto lowerBound = m_blockOutLinks.lower_bound(startAddress);
			auto upperBound = m_blockOutLinks.upper_bound(startAddress);
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				auto& blockLink = blockLinkIterator->second;
				if(blockLink.live) continue;
				auto referringBlock = m_blockLookup.FindBlockAt(blockLink.srcAddress);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->LinkBlock(blockLink.slot, block);
				blockLink.live = true;
			}
		}
	}

	virtual void PartitionFunction(uint32 startAddress)
	{
		uint32 endAddress = startAddress + MAX_BLOCK_SIZE;
		uint32 branchAddress = MIPS_INVALID_PC;
		for(uint32 address = startAddress; address < endAddress; address += 4)
		{
			uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
			auto branchType = m_context.m_pArch->IsInstructionBranch(&m_context, address, opcode);
			if(branchType == MIPS_BRANCH_NORMAL)
			{
				branchAddress = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
				endAddress = address + 4;
				//Check if the instruction in the delay slot (at address + 4) is a branch
				//If it is, don't include it in this block. This will make the behavior coherent between linked and non-linked blocks.
				//Branch instructions don't modify registers, it should be ok not to execute them if they are in a delay slot
				{
					uint32 endOpcode = m_context.m_pMemoryMap->GetInstruction(endAddress);
					auto endBranchType = m_context.m_pArch->IsInstructionBranch(&m_context, endAddress, endOpcode);
					if(endBranchType == MIPS_BRANCH_NORMAL)
					{
						endAddress = address;
					}
				}
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
		auto block = FindBlockStartingAt(startAddress);
		if(block->GetRecycleCount() < RECYCLE_NOLINK_THRESHOLD)
		{
			SetupBlockLinks(startAddress, endAddress, branchAddress);
		}
	}

	//Unlink and removes block from all of our bookkeeping structures
	void OrphanBlock(CBasicBlock* block)
	{
		auto orphanBlockLinkSlot =
		    [&](LINK_SLOT linkSlot) {
			    auto link = block->GetOutLink(linkSlot);
			    if(link != std::end(m_blockOutLinks))
			    {
				    if(link->second.live)
				    {
					    block->UnlinkBlock(linkSlot);
				    }
				    block->SetOutLink(linkSlot, std::end(m_blockOutLinks));
				    m_blockOutLinks.erase(link);
			    }
		    };
		orphanBlockLinkSlot(LINK_SLOT_NEXT);
		orphanBlockLinkSlot(LINK_SLOT_BRANCH);
	}

	void ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
	{
		//Widen scan range since blocks starting before the range can end in the range
		uint32 scanStart = static_cast<uint32>(std::max<int64>(0, static_cast<uint64>(start) - MAX_BLOCK_SIZE));
		uint32 scanEnd = end;
		assert(scanEnd > scanStart);

		std::set<CBasicBlock*> clearedBlocks;
		for(uint32 address = scanStart; address < scanEnd; address += instructionSize)
		{
			auto block = m_blockLookup.FindBlockAt(address);
			if(block->IsEmpty()) continue;
			if(block == protectedBlock) continue;
			if(!RangesOverlap(block->GetBeginAddress(), block->GetEndAddress(), start, end)) continue;
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
			auto lowerBound = m_blockOutLinks.lower_bound(block->GetBeginAddress());
			auto upperBound = m_blockOutLinks.upper_bound(block->GetBeginAddress());
			for(auto blockLinkIterator = lowerBound; blockLinkIterator != upperBound; blockLinkIterator++)
			{
				auto& blockLink = blockLinkIterator->second;
				if(!blockLink.live) continue;
				auto referringBlock = m_blockLookup.FindBlockAt(blockLink.srcAddress);
				if(referringBlock->IsEmpty()) continue;
				referringBlock->UnlinkBlock(blockLink.slot);
				blockLink.live = false;
			}
		}

		for(auto* clearedBlock : clearedBlocks)
		{
			m_blocks.erase(clearedBlock->shared_from_this());
		}
	}

	BlockStore m_blocks;
	BasicBlockPtr m_emptyBlock;
	BlockOutLinkMap m_blockOutLinks;
	CMIPS& m_context;
	uint32 m_maxAddress = 0;
	uint32 m_addressMask = 0;
	BLOCK_CATEGORY m_blockCategory = BLOCK_CATEGORY_UNKNOWN;

	BlockLookupType m_blockLookup;

#ifdef DEBUGGER_INCLUDED
	bool m_mustBreak = false;
	bool m_breakpointsDisabledOnce = false;
	int m_initQuota = 0;
#endif
};
