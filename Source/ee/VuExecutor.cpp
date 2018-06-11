#include "VuExecutor.h"
#include "VuBasicBlock.h"
#include <zlib.h>

CVuExecutor::CVuExecutor(CMIPS& context, uint32 maxAddress)
    : CMipsExecutor(context, maxAddress)
{
}

void CVuExecutor::Reset()
{
	m_cachedBlocks.clear();
	CMipsExecutor::Reset();
}

BasicBlockPtr CVuExecutor::BlockFactory(CMIPS& context, uint32 begin, uint32 end)
{
	uint32 blockSize = ((end - begin) + 4) / 4;
	uint32 blockSizeByte = blockSize * 4;
	uint32* blockMemory = reinterpret_cast<uint32*>(alloca(blockSizeByte));
	for(uint32 address = begin; address <= end; address += 8)
	{
		uint32 index = (address - begin) / 4;

		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		assert((index + 0) < blockSize);
		blockMemory[index + 0] = opcodeLo;
		assert((index + 1) < blockSize);
		blockMemory[index + 1] = opcodeHi;
	}

	uint32 checksum = crc32(0, reinterpret_cast<Bytef*>(blockMemory), blockSizeByte);

	auto equalRange = m_cachedBlocks.equal_range(checksum);
	for(; equalRange.first != equalRange.second; ++equalRange.first)
	{
		const auto& basicBlock(equalRange.first->second);
		if(basicBlock->GetBeginAddress() == begin)
		{
			if(basicBlock->GetEndAddress() == end)
			{
				return basicBlock;
			}
		}
	}

	auto result = std::make_shared<CVuBasicBlock>(context, begin, end);
	result->Compile();
	m_cachedBlocks.insert(std::make_pair(checksum, result));
	return result;
}

#define VU_UPPEROP_BIT_I (0x80000000)
#define VU_UPPEROP_BIT_E (0x40000000)

void CVuExecutor::PartitionFunction(uint32 startAddress)
{
	uint32 endAddress = startAddress + MAX_BLOCK_SIZE;
	for(uint32 address = startAddress; address < endAddress; address += 8)
	{
		uint32 addrLo = address + 0;
		uint32 addrHi = address + 4;
		uint32 lowerOp = m_context.m_pMemoryMap->GetInstruction(address + 0);
		uint32 upperOp = m_context.m_pMemoryMap->GetInstruction(address + 4);
		auto branchType = m_context.m_pArch->IsInstructionBranch(&m_context, addrLo, lowerOp);
		if(upperOp & VU_UPPEROP_BIT_E)
		{
			endAddress = address + 0xC;
			assert(branchType == MIPS_BRANCH_NONE);
			break;
		}
		else if(branchType == MIPS_BRANCH_NORMAL)
		{
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
}
