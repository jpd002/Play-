#include "VuExecutor.h"
#include "VuBasicBlock.h"
#include <zlib.h>

static const uint32 c_vuMaxAddress = 0x4000;

CVuExecutor::CVuExecutor(CMIPS& context) :
CMipsExecutor(context, c_vuMaxAddress)
{

}

CVuExecutor::~CVuExecutor()
{

}

void CVuExecutor::Reset()
{
	m_cachedBlocks.clear();
	CMipsExecutor::Reset();
}

CMipsExecutor::BasicBlockPtr CVuExecutor::BlockFactory(CMIPS& context, uint32 begin, uint32 end)
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
	m_cachedBlocks.insert(std::make_pair(checksum, result));
	return result;
}

void CVuExecutor::PartitionFunction(uint32 functionAddress)
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
		if(address >= c_vuMaxAddress)
		{
			endAddress = address;
			partitionPoints.insert(endAddress);
			break;
		}

		uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
		//If we find the E bit in an upper instruction
		if((address & 0x04) && (opcode & 0x40000000))
		{
			endAddress = address + 8;
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
			assert((address & 0x07) == 0x00);
			partitionPoints.insert(address + 0x10);
			uint32 target = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
			if(target > functionAddress && target < endAddress)
			{
				assert((target & 0x07) == 0x00);
				partitionPoints.insert(target);
			}
		}

		//Check if there's a block already exising that this address
		if(address != endAddress)
		{
			CBasicBlock* possibleBlock = FindBlockStartingAt(address);
			if(possibleBlock != NULL)
			{
				assert(possibleBlock->GetEndAddress() <= endAddress);
				//Add its beginning and end in the partition points
				partitionPoints.insert(possibleBlock->GetBeginAddress());
				partitionPoints.insert(possibleBlock->GetEndAddress() + 4);
			}
		}
	}

	uint32 currentPoint = MIPS_INVALID_PC;
	for(const auto& point : partitionPoints)
	{
		if(currentPoint != MIPS_INVALID_PC)
		{
			uint32 beginAddress = currentPoint;
			uint32 endAddress = point - 4;
			//Sanity checks
			assert((beginAddress & 0x07) == 0x00);
			assert((endAddress & 0x07) == 0x04);
			CreateBlock(beginAddress, endAddress);
		}
		currentPoint = point;
	}
/*
	//Convenient cutting for debugging purposes
	for(uint32 address = functionAddress; address <= endAddress; address += 8)
	{
		uint32 beginAddress = address;
		uint32 endAddress = address + 4;
		//Sanity checks
		assert((beginAddress & 0x07) == 0x00);
		assert((endAddress & 0x07) == 0x04);
		CreateBlock(beginAddress, endAddress);
	}
*/
}
