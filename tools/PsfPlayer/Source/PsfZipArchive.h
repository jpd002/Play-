#ifndef _PSF_ZIP_ARCHIVE_H_
#define _PSF_ZIP_ARCHIVE_H_

#include "PsfArchive.h"
#include "StdStream.h"
#include "zip/ZipArchiveReader.h"

class CPsfZipArchive : public CPsfArchive
{
public:
										CPsfZipArchive();
	virtual								~CPsfZipArchive();
	
	virtual void						Open(const char*);
	virtual void						ReadFileContents(const char*, void*, unsigned int);

private:
	Framework::CStdStream*				m_inputFile;
	Framework::CZipArchiveReader*		m_archive;
};

#endif
