#include "PsfArchive.h"
#include "PsfRarArchive.h"
#include "PsfZipArchive.h"
#include "stricmp.h"

CPsfArchive::PsfArchivePtr CPsfArchive::CreateFromPath(const boost::filesystem::path& filePath)
{
	auto extension = filePath.extension().string();
	PsfArchivePtr result;
	if(!strcmp(extension.c_str(), ".zip"))
	{
		result = std::make_unique<CPsfZipArchive>();
	}
#ifdef RAR_SUPPORT
	else if(!strcmp(extension.c_str(), ".rar"))
	{
		result = std::make_unique<CPsfRarArchive>();
	}
#endif
	else
	{
		throw std::runtime_error("Unsupported archive type.");
	}
	result->Open(filePath);
	return result;
}

const CPsfArchive::FileList& CPsfArchive::GetFiles() const
{
	return m_files;
}

const CPsfArchive::FILEINFO* CPsfArchive::GetFileInfo(const char* path) const
{
	for(const auto& fileInfo : m_files)
	{
		if(!stricmp(fileInfo.name.c_str(), path))
		{
			return &fileInfo;
		}
	}
	return nullptr;
}
