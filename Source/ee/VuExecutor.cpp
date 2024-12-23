#include "VuExecutor.h"
#include "VuBasicBlock.h"
#include "VUShared.h"
#include "xxhash.h"

// clang-format off
const CVuExecutor::BLOCK_COMPILE_HINTS CVuExecutor::g_blockCompileHints[] =
{
	// Tri-Ace VU0 decompression code
	{ std::make_pair(uint128{0x3aa43f7c, 0xe932516c, 0x6786472d, 0xf5333e12}, 0x58U), VUShared::COMPILEHINT_USE_ACCURATE_ADDI },
};
// clang-format on

CVuExecutor::CVuExecutor(CMIPS& context, uint32 maxAddress)
    : CGenericMipsExecutor(context, maxAddress, BLOCK_CATEGORY_PS2_VU)
{
}

void CVuExecutor::Reset()
{
	m_cachedBlocks.clear();
	CGenericMipsExecutor::Reset();
}

BasicBlockPtr CVuExecutor::BlockFactory(CMIPS& context, uint32 begin, uint32 end)
{
	uint32 blockSize = ((end - begin) + 4) / 4;
	uint32 blockSizeByte = blockSize * 4;

	auto map = m_context.m_pMemoryMap->GetInstructionMap(begin);
	assert(m_context.m_pMemoryMap->GetInstructionMap(end) == map);
	uint32 localBegin = begin - map->nStart;
	auto blockMemory = reinterpret_cast<const uint32*>(reinterpret_cast<uint8*>(map->pPointer) + localBegin);

	auto xxHash = XXH3_128bits(blockMemory, blockSizeByte);
	uint128 hash;
	memcpy(&hash, &xxHash, sizeof(xxHash));
	static_assert(sizeof(hash) == sizeof(xxHash));
	auto blockKey = std::make_pair(hash, blockSizeByte);

	//Don't use the cached blocks of we have a breakpoint in our block range.
	bool hasBreakpoint = m_context.HasBreakpointInRange(begin, end);
	if(!hasBreakpoint)
	{
		auto beginBlockIterator = m_cachedBlocks.lower_bound(blockKey);
		auto endBlockIterator = m_cachedBlocks.upper_bound(blockKey);
		//Check if we have a block that has the same contents and the same range.
		for(auto blockIterator = beginBlockIterator; blockIterator != endBlockIterator; blockIterator++)
		{
			const auto& basicBlock(blockIterator->second);
			if(basicBlock->GetBeginAddress() == begin && basicBlock->GetEndAddress() == end)
			{
				return basicBlock;
			}
		}
		//Check if we have a block that has the same contents but not the same range. Reuse the code of that block if that's the case.
		if(beginBlockIterator != endBlockIterator)
		{
			auto result = std::make_shared<CVuBasicBlock>(context, begin, end, m_blockCategory);
			result->CopyFunctionFrom(beginBlockIterator->second);
			m_cachedBlocks.insert(std::make_pair(blockKey, result));
			return result;
		}
	}

	//Totally new block, build it from scratch
	auto result = std::make_shared<CVuBasicBlock>(context, begin, end, m_blockCategory);

	auto blockCompileHintsIterator = std::find_if(std::begin(g_blockCompileHints), std::end(g_blockCompileHints),
	                                              [&](const auto& item) { return item.blockKey == blockKey; });
	if(blockCompileHintsIterator != std::end(g_blockCompileHints))
	{
		result->AddBlockCompileHints(blockCompileHintsIterator->hints);
	}

	result->Compile();
	if(!hasBreakpoint)
	{
		m_cachedBlocks.insert(std::make_pair(blockKey, result));
	}
	return result;
}

void CVuExecutor::PartitionFunction(uint32 startAddress)
{
	uint32 endAddress = std::min<uint32>(startAddress + MAX_BLOCK_SIZE - 4, m_maxAddress - 4);
	uint32 branchAddress = MIPS_INVALID_PC;
	for(uint32 address = startAddress; address < endAddress; address += 8)
	{
		uint32 addrLo = address + 0;
		uint32 addrHi = address + 4;
		uint32 lowerOp = m_context.m_pMemoryMap->GetInstruction(addrLo);
		uint32 upperOp = m_context.m_pMemoryMap->GetInstruction(addrHi);
		auto branchType = m_context.m_pArch->IsInstructionBranch(&m_context, addrLo, lowerOp);
		if(upperOp & VUShared::VU_UPPEROP_BIT_E)
		{
			endAddress = address + 0xC;
			break;
		}
		else if(upperOp & (VUShared::VU_UPPEROP_BIT_D | VUShared::VU_UPPEROP_BIT_T))
		{
			endAddress = address + 0x04;
			break;
		}
		else if(branchType == MIPS_BRANCH_NORMAL)
		{
			branchAddress = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, addrLo, lowerOp);
			endAddress = address + 0xC;
			break;
		}
		else if(branchType == MIPS_BRANCH_NODELAY)
		{
			//Should never happen
			assert(false);
		}
	}
	assert((endAddress - startAddress) <= MAX_BLOCK_SIZE);
	CreateBlock(startAddress, endAddress);
	auto block = static_cast<CVuBasicBlock*>(FindBlockStartingAt(startAddress));
	if(block->IsLinkable())
	{
		SetupBlockLinks(startAddress, endAddress, branchAddress);
	}
}
