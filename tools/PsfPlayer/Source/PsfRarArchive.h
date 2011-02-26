#ifndef _PSF_RAR_ARCHIVE_H_
#define _PSF_RAR_ARCHIVE_H_

#include "PsfArchive.h"

class CPsfRarArchive : public CPsfArchive
{
public:
					CPsfRarArchive();
	virtual			~CPsfRarArchive();
	
	virtual void	Open(const char*);
	virtual void	ReadFileContents(const char*, void*, unsigned int);

private:
	void*			m_archive;
};

#endif
