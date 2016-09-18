#include <boost/filesystem/operations.hpp>
#include "MemoryCard.h"

namespace filesystem = boost::filesystem;

CMemoryCard::CMemoryCard(const filesystem::path& basePath)
: m_basePath(basePath)
{
	ScanSaves();
}

CMemoryCard::~CMemoryCard()
{

}

size_t CMemoryCard::GetSaveCount() const
{
	return m_saves.size();
}

const CSave* CMemoryCard::GetSaveByIndex(size_t index) const
{
	return m_saves[index].get();
}

filesystem::path CMemoryCard::GetBasePath() const
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
		filesystem::directory_iterator endIterator;
		for(filesystem::directory_iterator elementIterator(m_basePath);
			elementIterator != endIterator; elementIterator++)
		{
			filesystem::path element(*elementIterator);

			if(filesystem::is_directory(element))
			{
				filesystem::path iconSysPath = element / "icon.sys";

				//Check if 'icon.sys' exists in this directory
				if(filesystem::exists(iconSysPath))
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
