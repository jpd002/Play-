#include "PathDirectoryIterator.h"
#include <cstring>

using namespace Iop;
using namespace Iop::Ioman;

CPathDirectoryIterator::CPathDirectoryIterator(const fs::path& path)
    : m_iterator(path)
{
}

void CPathDirectoryIterator::ReadEntry(DIRENTRY* dirEntry)
{
	auto itemPath = m_iterator->path();
	auto name = itemPath.filename().string();
	strncpy(dirEntry->name, name.c_str(), Ioman::DIRENTRY::NAME_SIZE);
	dirEntry->name[Ioman::DIRENTRY::NAME_SIZE - 1] = 0;

	auto& stat = dirEntry->stat;
	memset(&stat, 0, sizeof(Ioman::STAT));
	if(fs::is_directory(itemPath))
	{
		stat.mode = STAT_MODE_DIR;
		stat.attr = 0x8427;
	}
	else
	{
		stat.mode = STAT_MODE_FILE;
		stat.loSize = fs::file_size(itemPath);
		stat.attr = 0x8497;
	}

	m_iterator++;
}

bool CPathDirectoryIterator::IsDone()
{
	return m_iterator == fs::directory_iterator();
}
