#include "PsfZipArchive.h"
#include "StdStreamUtils.h"
#include "make_unique.h"

void CPsfZipArchive::Open(const boost::filesystem::path& filePath)
{
	assert(m_inputFile.IsEmpty());
	assert(!m_archive);
	m_inputFile = Framework::CreateInputStdStream(filePath.native());
	m_archive = std::make_unique<Framework::CZipArchiveReader>(m_inputFile);
	for(const auto& fileHeaderPair : m_archive->GetFileHeaders())
	{
		const auto& fileHeader(fileHeaderPair.second);
		if(fileHeader.uncompressedSize == 0) continue;
		FILEINFO fileInfo;
		fileInfo.name = fileHeaderPair.first;
		fileInfo.length = fileHeader.uncompressedSize;
		m_files.push_back(fileInfo);
	}
}

void CPsfZipArchive::ReadFileContents(const char* fileName, void* buffer, unsigned int bufferLength)
{
	assert(m_archive);
	if(!m_archive)
	{
		throw std::runtime_error("Archive not opened!");
	}

	Framework::CZipArchiveReader::StreamPtr stream = m_archive->BeginReadFile(fileName);
	stream->Read(buffer, bufferLength);
}
