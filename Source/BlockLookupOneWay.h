#pragma once

#include "Types.h"
#include "BasicBlock.h"

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
