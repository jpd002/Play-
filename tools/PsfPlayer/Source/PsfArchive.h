#pragma once

#include <list>
#include <functional>
#include "filesystem_def.h"

class CPsfArchive
{
public:
	struct FILEINFO
	{
		std::string name;
		unsigned int length;
	};

	typedef std::list<FILEINFO> FileList;
	typedef FileList::const_iterator FileListIterator;
	typedef std::unique_ptr<CPsfArchive> PsfArchivePtr;

	virtual ~CPsfArchive() = default;

	static PsfArchivePtr CreateFromPath(const fs::path&);

	virtual void ReadFileContents(const char*, void*, unsigned int) = 0;

	const FileList& GetFiles() const;
	const FILEINFO* GetFileInfo(const char*) const;

protected:
	virtual void Open(const fs::path&) = 0;

	FileList m_files;
};
