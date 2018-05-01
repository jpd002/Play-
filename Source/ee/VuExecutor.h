#pragma once

#include <unordered_map>
#include "../MipsExecutor.h"

class CVuExecutor : public CMipsExecutor
{
public:
	CVuExecutor(CMIPS&, uint32);
	virtual ~CVuExecutor() = default;

	void Reset() override;

protected:
	typedef std::unordered_multimap<uint32, BasicBlockPtr> CachedBlockMap;

	BasicBlockPtr BlockFactory(CMIPS&, uint32, uint32) override;
	void PartitionFunction(uint32) override;

	CachedBlockMap m_cachedBlocks;
};
