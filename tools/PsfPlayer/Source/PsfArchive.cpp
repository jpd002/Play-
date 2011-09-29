#include "PsfArchive.h"
#include "PsfRarArchive.h"
#include "PsfZipArchive.h"
#include "stricmp.h"

CPsfArchive::CPsfArchive()
{

}

CPsfArchive::~CPsfArchive()
{

}

CPsfArchive* CPsfArchive::CreateFromPath(const boost::filesystem::path& filePath)
{
	std::string extension = filePath.extension().string();
	CPsfArchive* result(NULL);
	if(!strcmp(extension.c_str(), ".zip"))
	{
		result = new CPsfZipArchive();
	}
#ifdef RAR_SUPPORT
	else if(!strcmp(extension.c_str(), ".rar"))
	{
		result = new CPsfRarArchive();
	}
#endif
	else
	{
		throw std::runtime_error("Unsupported archive type.");
	}
	result->Open(filePath);
	return result;
}

CPsfArchive::FileListIterator CPsfArchive::GetFileInfo(const char* path) const
{
	for(CPsfArchive::FileListIterator fileIterator(m_files.begin());
		m_files.end() != fileIterator; fileIterator++)
	{
		const FILEINFO& fileInfo(*fileIterator);
		if(!stricmp(fileInfo.name.c_str(), path))
		{
			return fileIterator;
		}
	}
	return m_files.end();
}

CPsfArchive::FileListIterator CPsfArchive::GetFilesBegin() const
{
	return m_files.begin();
}

CPsfArchive::FileListIterator CPsfArchive::GetFilesEnd() const
{
	return m_files.end();
}
