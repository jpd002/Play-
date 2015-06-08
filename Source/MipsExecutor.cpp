#include "MipsExecutor.h"

static bool IsInsideRange(uint32 address, uint32 start, uint32 end)
{
	return (address >= start) && (address <= end);
}

CMipsExecutor::CMipsExecutor(CMIPS& context, uint32 maxAddress)
: m_context(context)
, m_subTableCount(0)
#ifdef DEBUGGER_INCLUDED
, m_breakpointsDisabledOnce(false)
#endif
{
	if(maxAddress < 0x10000)
	{
		maxAddress = 0x10000;
	}
	assert((maxAddress & 0xFFFF) == 0);
	if(maxAddress == 0)
	{
		m_subTableCount = 0x10000;
	}
	else
	{
		m_subTableCount = maxAddress / 0x10000;
	}
	m_blockTable = new CBasicBlock**[m_subTableCount];
	memset(m_blockTable, 0, sizeof(CBasicBlock**) * m_subTableCount);
}

CMipsExecutor::~CMipsExecutor()
{
	for(unsigned int i = 0; i < m_subTableCount; i++)
	{
		CBasicBlock** subTable = m_blockTable[i];
		if(subTable != NULL)
		{
			delete [] subTable;
		}
	}
	delete [] m_blockTable;
}

void CMipsExecutor::Reset()
{
	ClearActiveBlocks();
}

void CMipsExecutor::ClearActiveBlocks()
{
	for(unsigned int i = 0; i < m_subTableCount; i++)
	{
		CBasicBlock** subTable = m_blockTable[i];
		if(subTable != NULL)
		{
			delete [] subTable;
			m_blockTable[i] = NULL;
		}
	}
	
	m_blocks.clear();
}

void CMipsExecutor::ClearActiveBlocksInRange(uint32 start, uint32 end)
{
	ClearActiveBlocksInRangeInternal(start, end, nullptr);
}

void CMipsExecutor::ClearActiveBlocksInRangeInternal(uint32 start, uint32 end, CBasicBlock* protectedBlock)
{
	uint32 hiStart = start >> 16;
	uint32 hiEnd = end >> 16;

	//We need to increase the range to make sure we catch any
	//block that are straddling table boundaries
	if(hiStart != 0) hiStart--;
	if(hiEnd != (m_subTableCount - 1)) hiEnd++;

	std::set<CBasicBlock*> blocksToDelete;

	for(uint32 hi = hiStart; hi <= hiEnd; hi++)
	{
		CBasicBlock** table = m_blockTable[hi];
		if(table == nullptr) continue;

		for(uint32 lo = 0; lo < 0x10000; lo += 4)
		{
			uint32 tableAddress = (hi << 16) | lo;
			auto block = table[lo / 4];
			if(block == nullptr) continue;
			if(block == protectedBlock) continue;
			if(!IsInsideRange(block->GetBeginAddress(), start, end) 
				&& !IsInsideRange(block->GetEndAddress(), start, end)) continue;
			table[lo / 4] = nullptr;
			blocksToDelete.insert(block);
		}
	}

	if(!blocksToDelete.empty())
	{
		m_blocks.remove_if([&] (const BasicBlockPtr& block) { return blocksToDelete.find(block.get()) != std::end(blocksToDelete); });
	}
}

int CMipsExecutor::Execute(int cycles)
{
	CBasicBlock* block(nullptr);
	while(cycles > 0)
	{
		uint32 address = m_context.m_pAddrTranslator(&m_context, m_context.m_State.nPC);
		if(!block || address != block->GetBeginAddress())
		{
			block = FindBlockStartingAt(address);
			if(block == NULL)
			{
				//We need to partition the space and compile the blocks
				PartitionFunction(address);
				block = FindBlockStartingAt(address);
				if(block == NULL)
				{
					throw std::runtime_error("Couldn't create block starting at address.");
				}
			}
			if(!block->IsCompiled())
			{
				block->Compile();
			}
		}
		else if(block != NULL)
		{
			block->SetSelfLoopCount(block->GetSelfLoopCount() + 1);
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

#ifdef DEBUGGER_INCLUDED

bool CMipsExecutor::MustBreak() const
{
	uint32 currentPc = m_context.m_pAddrTranslator(&m_context, m_context.m_State.nPC);
	CBasicBlock* block = FindBlockAt(currentPc);
	for(auto breakPointIterator(m_context.m_breakpoints.begin());
		breakPointIterator != m_context.m_breakpoints.end(); breakPointIterator++)
	{
		uint32 breakPointAddress = *breakPointIterator;
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

CBasicBlock* CMipsExecutor::FindBlockAt(uint32 address) const
{
	uint32 hiAddress = address >> 16;
	uint32 loAddress = address & 0xFFFF;
	assert(hiAddress < m_subTableCount);
	CBasicBlock**& subTable = m_blockTable[hiAddress];
	if(subTable == NULL) return NULL;
	CBasicBlock* result = subTable[loAddress / 4];
	return result;
}

CBasicBlock* CMipsExecutor::FindBlockStartingAt(uint32 address) const
{
	uint32 hiAddress = address >> 16;
	uint32 loAddress = address & 0xFFFF;
	assert(hiAddress < m_subTableCount);
	CBasicBlock**& subTable = m_blockTable[hiAddress];
	if(subTable == NULL) return NULL;
	CBasicBlock* result = subTable[loAddress / 4];
	if((address != 0) && (FindBlockAt(address - 4) == result))
	{
		return NULL;
	}
	return result;
}

void CMipsExecutor::CreateBlock(uint32 start, uint32 end)
{
	{
		CBasicBlock* block = FindBlockAt(start);
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
				assert(FindBlockAt(start) == NULL);
			}
			else if(otherBegin == start)
			{
				DeleteBlock(block);
				CreateBlock(end + 4, otherEnd);
				assert(FindBlockAt(end) == NULL);
			}
			else
			{
				//Delete the currently existing block otherwise
				printf("MipsExecutor: Warning. Deleting block at %0.8X.\r\n", block->GetEndAddress());
				DeleteBlock(block);
			}
		}
	}
	assert(FindBlockAt(end) == NULL);
	{
		BasicBlockPtr block = BlockFactory(m_context, start, end);
		for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += 4)
		{
			uint32 hiAddress = address >> 16;
			uint32 loAddress = address & 0xFFFF;
			assert(hiAddress < m_subTableCount);
			CBasicBlock**& subTable = m_blockTable[hiAddress];
			if(subTable == NULL)
			{
				const uint32 subTableSize = 0x10000 / 4;
				subTable = new CBasicBlock*[subTableSize];
				memset(subTable, 0, sizeof(CBasicBlock*) * subTableSize);
			}
			assert(subTable[loAddress / 4] == NULL);
			subTable[loAddress / 4] = block.get();
		}
		m_blocks.push_back(std::move(block));
	}
}

void CMipsExecutor::DeleteBlock(CBasicBlock* block)
{
	for(uint32 address = block->GetBeginAddress(); address <= block->GetEndAddress(); address += 4)
	{
		uint32 hiAddress = address >> 16;
		uint32 loAddress = address & 0xFFFF;
		assert(hiAddress < m_subTableCount);
		CBasicBlock**& subTable = m_blockTable[hiAddress];
		assert(subTable != NULL);
		assert(subTable[loAddress / 4] != NULL);
		subTable[loAddress / 4] = NULL;
	}

	//Remove block from our lists
	auto blockIterator = std::find_if(std::begin(m_blocks), std::end(m_blocks), [&] (const BasicBlockPtr& blockPtr) { return blockPtr.get() == block; });
	assert(blockIterator != std::end(m_blocks));
	m_blocks.erase(blockIterator);
}

CMipsExecutor::BasicBlockPtr CMipsExecutor::BlockFactory(CMIPS& context, uint32 start, uint32 end)
{
	return std::make_shared<CBasicBlock>(context, start, end);
}

void CMipsExecutor::PartitionFunction(uint32 functionAddress)
{
	typedef std::set<uint32> PartitionPointSet;
	uint32 endAddress = 0;
	PartitionPointSet partitionPoints;

	//Insert begin point
	partitionPoints.insert(functionAddress);

	//Find the end
	for(uint32 address = functionAddress; ; address += 4)
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
			pointIterator != partitionPoints.end(); pointIterator++)
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
					pointIterator--;
					continue;
				}
			}
			currentPoint = *pointIterator;
		}
	}

	//Create blocks
	{
		uint32 currentPoint = -1;
		for(PartitionPointSet::const_iterator pointIterator(partitionPoints.begin());
			pointIterator != partitionPoints.end(); pointIterator++)
		{
			if(currentPoint != -1)
			{
				CreateBlock(currentPoint, *pointIterator - 4);
			}
			currentPoint = *pointIterator;
		}
	}
}
