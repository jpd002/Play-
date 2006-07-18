#ifndef _MEMORYCARD_H_
#define _MEMORYCARD_H_

#include <boost/filesystem/path.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include "../saves/Save.h"

class CMemoryCard
{
public:
	typedef boost::ptr_list<CSave>	SaveList;
	typedef SaveList::iterator		SaveIterator;

									CMemoryCard(boost::filesystem::path&);
									~CMemoryCard();

	SaveIterator					GetSavesBegin();
	SaveIterator					GetSavesEnd();
	const char*						GetBasePath();
	void							RefreshContents();

private:

	void							ScanSaves();

	SaveList						m_Saves;
	boost::filesystem::path			m_BasePath;

};

#endif
