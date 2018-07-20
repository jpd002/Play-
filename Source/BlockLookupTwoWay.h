#pragma once

#include <cstring>
#include "Types.h"
#include "BasicBlock.h"

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
