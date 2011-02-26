#include "PsfStreamProvider.h"
#include "StdStream.h"
#include "MemStream.h"

CPsfStreamProvider*	CreatePsfStreamProvider(const char* archivePath)
{
	if(archivePath == NULL)
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
Framework::CStream*	CPhysicalPsfStreamProvider::GetStreamForPath(const char* path)
{
	return new Framework::CStdStream(path, "rb");
}

//CArchivePsfStreamProvider
//----------------------------------------------------------
CArchivePsfStreamProvider::CArchivePsfStreamProvider(const char* path)
{
	m_archive = CPsfArchive::CreateFromPath(path);
}

CArchivePsfStreamProvider::~CArchivePsfStreamProvider()
{
	delete m_archive;
}

Framework::CStream* CArchivePsfStreamProvider::GetStreamForPath(const char* path)
{
	CPsfArchive::FileListIterator fileInfoIterator(m_archive->GetFileInfo(path));
	assert(fileInfoIterator != m_archive->GetFilesEnd());
	if(fileInfoIterator == m_archive->GetFilesEnd())
	{
		throw std::runtime_error("Couldn't find file in archive.");
	}
	const CPsfArchive::FILEINFO& fileInfo(*fileInfoIterator);
	Framework::CMemStream* result(new Framework::CMemStream());
	result->Allocate(fileInfo.length);
	m_archive->ReadFileContents(path, result->GetBuffer(), fileInfo.length);
	return result;
}
