#include "OpticalMediaDirectoryIterator.h"
#include <cstring>

using namespace Iop;
using namespace Ioman;

COpticalMediaDirectoryIterator::COpticalMediaDirectoryIterator(Framework::CStream* directoryStream)
    : m_directoryStream(directoryStream)
{
	SeekToNextEntry();
}

COpticalMediaDirectoryIterator::~COpticalMediaDirectoryIterator()
{
	delete m_directoryStream;
}

void COpticalMediaDirectoryIterator::ReadEntry(DIRENTRY* dirEntry)
{
	const char* name = m_currentRecord.GetName();
	strncpy(dirEntry->name, name, Ioman::DIRENTRY::NAME_SIZE);
	dirEntry->name[Ioman::DIRENTRY::NAME_SIZE - 1] = 0;

	auto& stat = dirEntry->stat;
	memset(&stat, 0, sizeof(Ioman::STAT));
	if(m_currentRecord.IsDirectory())
	{
		stat.mode = STAT_MODE_DIR;
		stat.attr = 0x8427;
	}
	else
	{
		stat.mode = STAT_MODE_FILE;
		stat.loSize = m_currentRecord.GetDataLength();
		stat.attr = 0x8497;
	}

	SeekToNextEntry();
}

bool COpticalMediaDirectoryIterator::IsDone()
{
	return m_currentRecord.GetLength() == 0;
}

void COpticalMediaDirectoryIterator::SeekToNextEntry()
{
	while(1)
	{
		m_currentRecord = ISO9660::CDirectoryRecord(m_directoryStream);
		if(m_currentRecord.GetLength() == 0) break;
		const char* name = m_currentRecord.GetName();
		// Skip unnecessary entries
		if(name[0] == 0x00) continue;
		if(name[0] == 0x01) continue;
		break;
	}
}
