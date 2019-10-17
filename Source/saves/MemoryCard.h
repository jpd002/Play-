#pragma once

#include <memory>
#include <vector>
#include "filesystem_def.h"
#include "Save.h"

class CMemoryCard
{
public:
	typedef std::shared_ptr<CSave> SavePtr;
	typedef std::vector<SavePtr> SaveList;

	CMemoryCard(const fs::path&);
	virtual ~CMemoryCard() = default;

	size_t GetSaveCount() const;
	const CSave* GetSaveByIndex(size_t) const;

	fs::path GetBasePath() const;
	void RefreshContents();

private:
	void ScanSaves();

	SaveList m_saves;
	fs::path m_basePath;
};
