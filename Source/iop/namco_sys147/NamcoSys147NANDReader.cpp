#include "NamcoSys147NANDReader.h"
#include <cassert>
#include "maybe_unused.h"
#include "PtrStream.h"

using namespace Namco;

CSys147NANDReader::CSys147NANDReader(Framework::CStream& stream, uint32 baseSector)
	: m_stream(stream)
	, m_baseSector(baseSector)
{
	m_baseSector = baseSector;
}

CSys147NANDReader::Directory CSys147NANDReader::ReadDirectory(uint32 dirSector)
{
	uint8 sectorData[m_dataSize];
	ReadSector(dirSector, sectorData);
	Framework::CPtrStream dirStream(sectorData, m_dataSize);
	DIRHEADER header;
	dirStream.Read(&header, sizeof(DIRHEADER));
	assert(!strcmp(header.signature, "S147ROM"));
	//Make sure our dir doesn't span multiple sectors
	assert(((header.entryCount + 1) * 0x20) <= m_dataSize);

	Directory result;
	for(int i = 0; i < header.entryCount; i++)
	{
		DIRENTRY entry;
		dirStream.Read(&entry, sizeof(DIRENTRY));
		result.push_back(entry);
	}
	return result;
}

std::vector<uint8> CSys147NANDReader::ReadFile(uint32 fileSector, uint32 fileSize)
{
	std::vector<uint8> result;
	result.reserve(fileSize);
	while(fileSize != 0)
	{
		uint8 sectorData[m_dataSize];
		ReadSector(fileSector, sectorData);
		uint32 toWrite = std::min<uint32>(fileSize, m_dataSize);
		result.insert(result.end(), sectorData, sectorData + toWrite);
		fileSize -= toWrite;
		fileSector++;
	}
	return result;
}

void CSys147NANDReader::ReadSector(uint32 sectorIndex, void* buffer)
{
	m_stream.Seek((m_baseSector + sectorIndex) * m_sectorSize, Framework::STREAM_SEEK_SET);
	FRAMEWORK_MAYBE_UNUSED auto readAmount = m_stream.Read(buffer, m_dataSize);
	assert(readAmount == m_dataSize);
}
