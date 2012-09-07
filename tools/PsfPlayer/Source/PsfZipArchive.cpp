#include "PsfZipArchive.h"
#include "StdStreamUtils.h"

CPsfZipArchive::CPsfZipArchive()
: m_inputFile(nullptr)
, m_archive(nullptr)
{

}

CPsfZipArchive::~CPsfZipArchive()
{
	if(m_archive)
	{
		delete m_archive;
	}

	if(m_inputFile)
	{
		delete m_inputFile;
	}
}

void CPsfZipArchive::Open(const boost::filesystem::path& filePath)
{
	m_inputFile = new Framework::CStdStream(Framework::CreateInputStdStream(filePath.native()));
	m_archive = new Framework::CZipArchiveReader(*m_inputFile);

	for(auto fileHeaderIterator(std::begin(m_archive->GetFileHeaders()));
		fileHeaderIterator != std::end(m_archive->GetFileHeaders()); fileHeaderIterator++)
	{
		const Framework::Zip::ZIPDIRFILEHEADER& fileHeader(fileHeaderIterator->second);
		if(fileHeader.uncompressedSize == 0) continue;
		FILEINFO fileInfo;
		fileInfo.name = fileHeaderIterator->first;
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
