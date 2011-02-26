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

CPsfArchive* CPsfArchive::CreateFromPath(const char* path)
{
	std::string pathString(path);
	std::string extension;
	std::string::size_type dotPosition = pathString.find('.');
	if(dotPosition != std::string::npos)
	{
		extension = std::string(pathString.begin() + dotPosition + 1, pathString.end());
	}
	CPsfArchive* result(NULL);
	if(!strcmp(extension.c_str(), "zip"))
	{
		result = new CPsfZipArchive();
	}
#ifdef RAR_SUPPORT
	else if(!strcmp(extension.c_str(), "rar"))
	{
		result = new CPsfRarArchive();
	}
#endif
	else
	{
		throw std::runtime_error("Unsupported archive type.");
	}
	result->Open(path);
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
