#include "PsfStreamProvider.h"
#include "StdStream.h"
#include "MemStream.h"
#include "StdStreamUtils.h"

CPsfStreamProvider*	CreatePsfStreamProvider(const boost::filesystem::path& archivePath)
{
	if(archivePath.empty())
	{
		return new CPhysicalPsfStreamProvider();
	}
	else
	{
		return new CArchivePsfStreamProvider(archivePath);
	}
}

//CPhysicalPsfStreamProvider
//----------------------------------------------------------
Framework::CStream*	CPhysicalPsfStreamProvider::GetStreamForPath(const boost::filesystem::path& path)
{
	return Framework::CreateInputStdStream(path.native());
}

//CArchivePsfStreamProvider
//----------------------------------------------------------
CArchivePsfStreamProvider::CArchivePsfStreamProvider(const boost::filesystem::path& path)
{
	m_archive = CPsfArchive::CreateFromPath(path);
}

CArchivePsfStreamProvider::~CArchivePsfStreamProvider()
{
	delete m_archive;
}

Framework::CStream* CArchivePsfStreamProvider::GetStreamForPath(const boost::filesystem::path& path)
{
	std::string pathString = path.string().c_str();
	std::replace(pathString.begin(), pathString.end(), '\\', '/');
	CPsfArchive::FileListIterator fileInfoIterator(m_archive->GetFileInfo(pathString.c_str()));
	assert(fileInfoIterator != m_archive->GetFilesEnd());
	if(fileInfoIterator == m_archive->GetFilesEnd())
	{
		throw std::runtime_error("Couldn't find file in archive.");
	}
	const CPsfArchive::FILEINFO& fileInfo(*fileInfoIterator);
	Framework::CMemStream* result(new Framework::CMemStream());
	result->Allocate(fileInfo.length);
	m_archive->ReadFileContents(pathString.c_str(), result->GetBuffer(), fileInfo.length);
	return result;
}
