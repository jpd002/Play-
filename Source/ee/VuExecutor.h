#ifndef _VUEXECUTOR_H_
#define _VUEXECUTOR_H_

#include <unordered_map>
#include "../MipsExecutor.h"

class CVuExecutor : public CMipsExecutor
{
public:
							CVuExecutor(CMIPS&);
	virtual					~CVuExecutor();

	virtual void			Reset();

protected:
	typedef std::unordered_multimap<uint32, BasicBlockPtr> CachedBlockMap;

	virtual BasicBlockPtr	BlockFactory(CMIPS&, uint32, uint32);
	virtual void			PartitionFunction(uint32);

	CachedBlockMap			m_cachedBlocks;
};

#endif
