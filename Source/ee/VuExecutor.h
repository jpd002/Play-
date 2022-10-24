#pragma once

#include <map>
#include "../GenericMipsExecutor.h"

class CVuExecutor : public CGenericMipsExecutor<BlockLookupOneWay, 8>
{
public:
	CVuExecutor(CMIPS&, uint32);
	virtual ~CVuExecutor() = default;

	void Reset() override;

protected:
	typedef std::pair<uint128, uint32> CachedBlockKey;
	typedef std::multimap<CachedBlockKey, BasicBlockPtr> CachedBlockMap;
	CachedBlockMap m_cachedBlocks;

	BasicBlockPtr BlockFactory(CMIPS&, uint32, uint32) override;
	void PartitionFunction(uint32) override;
};
