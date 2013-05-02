#include "PsfStreamProvider.h"
#include "StdStream.h"
#include "MemStream.h"
#include "StdStreamUtils.h"

std::unique_ptr<CPsfStreamProvider> CreatePsfStreamProvider(const boost::filesystem::path& archivePath)
{
	if(archivePath.empty())
	{
		return std::unique_ptr<CPsfStreamProvider>(new CPhysicalPsfStreamProvider());
	}
	else
	{
		return std::unique_ptr<CPsfStreamProvider>(new CArchivePsfStreamProvider(archivePath));
	}
}

//CPhysicalPsfStreamProvider
//----------------------------------------------------------
Framework::CStream*	CPhysicalPsfStreamProvider::GetStreamForPath(const boost::filesystem::path& path)
{
	return new Framework::CStdStream(Framework::CreateInputStdStream(path.native()));
}

//CArchivePsfStreamProvider
//----------------------------------------------------------
CArchivePsfStreamProvider::CArchivePsfStreamProvider(const boost::filesystem::path& path)
{
	m_archive = CPsfArchive::CreateFromPath(path);
}

CArchivePsfStreamProvider::~CArchivePsfStreamProvider()
{

}

Framework::CStream* CArchivePsfStreamProvider::GetStreamForPath(const boost::filesystem::path& path)
{
	std::string pathString = path.string().c_str();
	std::replace(pathString.begin(), pathString.end(), '\\', '/');
	CPsfArchive::FileListIterator fileInfoIterator(m_archive->GetFileInfo(pathString.c_str()));
	assert(fileInfoIterator != std::end(m_archive->GetFiles()));
	if(fileInfoIterator == std::end(m_archive->GetFiles()))
	{
		throw std::runtime_error("Couldn't find file in archive.");
	}
	const CPsfArchive::FILEINFO& fileInfo(*fileInfoIterator);
	Framework::CMemStream* result(new Framework::CMemStream());
	result->Allocate(fileInfo.length);
	m_archive->ReadFileContents(pathString.c_str(), result->GetBuffer(), fileInfo.length);
	return result;
}
