#include "MipsExecutor.h"

CMipsExecutor::CMipsExecutor(CMIPS& context, uint32 maxAddress)
    : m_context(context)
    , m_maxAddress(maxAddress)
    , m_blockLookup(maxAddress)
{
}

void CMipsExecutor::Reset()
{
	ClearActiveBlocks();
}

void CMipsExecutor::ClearActiveBlocks()
{
	m_blockLookup.Clear();
	m_blocks.clear();
}

void CMipsExecutor::ClearActiveBlocksInRange(uint32 start, uint32 end)
{
	ClearActiveBlocksInRangeInternal(start, end, nullptr);
}

void CMipsExecutor::ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
{
	auto clearedBlocks = m_blockLookup.ClearInRange(start, end, protectedBlock);
	if(!clearedBlocks.empty())
	{
		m_blocks.remove_if([&](const BasicBlockPtr& block) { return clearedBlocks.find(block.get()) != std::end(clearedBlocks); });
	}
}

#ifdef DEBUGGER_INCLUDED

bool CMipsExecutor::MustBreak() const
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

void CMipsExecutor::DisableBreakpointsOnce()
{
	m_breakpointsDisabledOnce = true;
}

#endif

CBasicBlock* CMipsExecutor::FindBlockStartingAt(uint32 address) const
{
	auto result = m_blockLookup.FindBlockAt(address);
	if(result && (result->GetBeginAddress() != address)) return nullptr;
	return result;
}

void CMipsExecutor::CreateBlock(uint32 start, uint32 end)
{
	{
		auto block = m_blockLookup.FindBlockAt(start);
		if(block)
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
				assert(!m_blockLookup.FindBlockAt(start));
			}
			else if(otherBegin == start)
			{
				DeleteBlock(block);
				CreateBlock(end + 4, otherEnd);
				assert(!m_blockLookup.FindBlockAt(end));
			}
			else
			{
				//Delete the currently existing block otherwise
				printf("MipsExecutor: Warning. Deleting block at %08X.\r\n", block->GetEndAddress());
				DeleteBlock(block);
			}
		}
	}
	assert(!m_blockLookup.FindBlockAt(end));
	{
		auto block = BlockFactory(m_context, start, end);
		m_blockLookup.AddBlock(block.get());
		m_blocks.push_back(std::move(block));
	}
}

void CMipsExecutor::DeleteBlock(CBasicBlock* block)
{
	m_blockLookup.DeleteBlock(block);

	//Remove block from our lists
	auto blockIterator = std::find_if(std::begin(m_blocks), std::end(m_blocks), [&](const BasicBlockPtr& blockPtr) { return blockPtr.get() == block; });
	assert(blockIterator != std::end(m_blocks));
	m_blocks.erase(blockIterator);
}

CMipsExecutor::BasicBlockPtr CMipsExecutor::BlockFactory(CMIPS& context, uint32 start, uint32 end)
{
	auto result = std::make_shared<CBasicBlock>(context, start, end);
	result->Compile();
	return result;
}

void CMipsExecutor::PartitionFunction(uint32 functionAddress)
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
			printf("MipsExecutor: Warning. Found no JR after a big distance.\r\n");
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
			CBasicBlock* possibleBlock = FindBlockStartingAt(address);
			if(possibleBlock)
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
