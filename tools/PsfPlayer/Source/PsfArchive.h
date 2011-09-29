#ifndef _PSFARCHIVE_H_
#define _PSFARCHIVE_H_

#include <list>
#include <functional>
#include <boost/filesystem/path.hpp>

class CPsfArchive
{
public:
	struct FILEINFO
	{
		std::string		name;
		unsigned int	length;
	};

	typedef std::list<FILEINFO> FileList;
	typedef FileList::const_iterator FileListIterator;

							CPsfArchive();
	virtual					~CPsfArchive();

	static CPsfArchive*		CreateFromPath(const boost::filesystem::path&);

	virtual void			ReadFileContents(const char*, void*, unsigned int) = 0;

	FileListIterator		GetFileInfo(const char*) const;
	FileListIterator		GetFilesBegin() const;
	FileListIterator		GetFilesEnd() const;

protected:
	virtual void			Open(const boost::filesystem::path&) = 0;

	FileList				m_files;
};

#endif
