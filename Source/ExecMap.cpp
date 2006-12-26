#include <string.h>
#include <assert.h>
#include "PtrMacro.h"
#include "ExecMap.h"

CExecMap::CExecMap(uint32 nStart, uint32 nEnd, uint32 nGranularity)
{
	m_nStart		= nStart;
	m_nEnd			= nEnd;
	m_nGranularity	= nGranularity;

	m_nCount		= (m_nEnd - m_nStart) / m_nGranularity;

	m_pBlock = new CCacheBlock*[m_nCount];
	memset(m_pBlock, 0, sizeof(CCacheBlock*) * m_nCount);
}

CExecMap::~CExecMap()
{
	InvalidateBlocks();
	DELETEPTR(m_pBlock);
}

CCacheBlock* CExecMap::CreateBlock(uint32 nAddress)
{
	CCacheBlock** pBlock;
	pBlock = GetBlockAddress(nAddress);
	if(pBlock == NULL) return NULL;

	if((*pBlock) == NULL)
	{
		nAddress &= ~(m_nGranularity - 1);
		(*pBlock) = new CCacheBlock(nAddress, nAddress + m_nGranularity);
	}

	return (*pBlock);
}

CCacheBlock* CExecMap::FindBlock(uint32 nAddress)
{
	CCacheBlock** pBlock;
	pBlock = GetBlockAddress(nAddress);
	return (pBlock == NULL) ? NULL : (*pBlock);
}

void CExecMap::InvalidateBlock(uint32 nAddress)
{
	CCacheBlock** pBlock;
	pBlock = GetBlockAddress(nAddress);
	if(pBlock == NULL) return;
	DELETEPTR(*pBlock);
}

void CExecMap::InvalidateBlocks()
{
	unsigned int i;

	for(i = 0; i < m_nCount; i++)
	{
		DELETEPTR(m_pBlock[i]);
	}
}

CCacheBlock** CExecMap::GetBlockAddress(uint32 nAddress)
{
	uint32 nBlock;

	if(nAddress >= m_nEnd)
	{
		assert(0);
		return NULL;
	}
	if(nAddress < m_nStart) 
	{
		assert(0);
		return NULL;
	}

	nBlock = nAddress;
	nBlock -= m_nStart;
	nBlock /= m_nGranularity;

	return &m_pBlock[nBlock];
}
