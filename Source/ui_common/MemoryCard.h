#pragma once

#include <memory>
#include <boost/filesystem/path.hpp>
#include "../saves/Save.h"

class CMemoryCard
{
public:
	typedef std::shared_ptr<CSave>		SavePtr;
	typedef std::vector<SavePtr>		SaveList;

									CMemoryCard(const boost::filesystem::path&);
	virtual							~CMemoryCard();

	size_t							GetSaveCount() const;
	const CSave*					GetSaveByIndex(size_t) const;

	boost::filesystem::path			GetBasePath() const;
	void							RefreshContents();

private:

	void							ScanSaves();

	SaveList						m_saves;
	boost::filesystem::path			m_basePath;

};
