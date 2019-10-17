#include "MemoryCard.h"

CMemoryCard::CMemoryCard(const fs::path& basePath)
    : m_basePath(basePath)
{
	ScanSaves();
}

size_t CMemoryCard::GetSaveCount() const
{
	return m_saves.size();
}

const CSave* CMemoryCard::GetSaveByIndex(size_t index) const
{
	return m_saves[index].get();
}

fs::path CMemoryCard::GetBasePath() const
{
	return m_basePath;
}

void CMemoryCard::RefreshContents()
{
	m_saves.clear();
	ScanSaves();
}

void CMemoryCard::ScanSaves()
{
	try
	{
		fs::directory_iterator endIterator;
		for(fs::directory_iterator elementIterator(m_basePath);
		    elementIterator != endIterator; elementIterator++)
		{
			fs::path element(*elementIterator);

			if(fs::is_directory(element))
			{
				fs::path iconSysPath = element / "icon.sys";

				//Check if 'icon.sys' exists in this directory
				if(fs::exists(iconSysPath))
				{
					try
					{
						m_saves.push_back(std::make_shared<CSave>(element));
					}
					catch(const std::exception& exception)
					{
						printf("Failed to create save: %s\r\n", exception.what());
					}
				}
			}
		}
	}
	catch(const std::exception& exception)
	{
		printf("Exception caught in CMemoryCard::ScanSaves: %s\r\n", exception.what());
	}
}
