#include <boost/filesystem/operations.hpp>
#include "MemoryCard.h"

using namespace boost;
using namespace std;

CMemoryCard::CMemoryCard(filesystem::path& BasePath) :
m_BasePath(BasePath)
{
	ScanSaves();
}

CMemoryCard::~CMemoryCard()
{
	
}

size_t CMemoryCard::GetSaveCount() const
{
	return m_Saves.size();
}

const CSave* CMemoryCard::GetSaveByIndex(size_t nIndex) const
{
	return &m_Saves[nIndex];
}

const char* CMemoryCard::GetBasePath()
{
	return m_BasePath.string().c_str();
}

void CMemoryCard::RefreshContents()
{
	m_Saves.clear();
	ScanSaves();
}

void CMemoryCard::ScanSaves()
{
	filesystem::directory_iterator itEnd;
	
	try
	{
		for(filesystem::directory_iterator itElement(m_BasePath);
			itElement != itEnd;
			itElement++)
		{
            filesystem::path Element(*itElement);

			if(filesystem::is_directory(Element))
			{
				filesystem::path IconSysPath;
                IconSysPath = Element / "icon.sys";

				//Check if 'icon.sys' exists in this directory
				if(filesystem::exists(IconSysPath))
				{
					//Create new Save
					m_Saves.push_back(new CSave(Element));
				}
			}
		}
	}
	catch(const exception& Exception)
	{
		printf("Exception caught in CMemoryCard::ScanSaves: %s\r\n", Exception.what());
	}
}
