#ifndef _EXECMAP_H_
#define _EXECMAP_H_

#include "CacheBlock.h"

class CExecMap
{
public:
							CExecMap(uint32, uint32, uint32);
							~CExecMap();
	CCacheBlock*			CreateBlock(uint32);
	CCacheBlock*			FindBlock(uint32);
	void					InvalidateBlocks();

private:
	uint32					m_nStart;
	uint32					m_nEnd;
	uint32					m_nGranularity;
	uint32					m_nCount;
	CCacheBlock**			m_pBlock;

};

#endif