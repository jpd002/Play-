#ifndef _PSFSTREAMPROVIDER_H_
#define _PSFSTREAMPROVIDER_H_

#include "PsfArchive.h"
#include "Stream.h"

class CPsfStreamProvider
{
public:
	virtual							~CPsfStreamProvider() {}
	virtual Framework::CStream*		GetStreamForPath(const char*) = 0;
};

class CPhysicalPsfStreamProvider : public CPsfStreamProvider
{
public:
	virtual							~CPhysicalPsfStreamProvider() {}

	Framework::CStream*				GetStreamForPath(const char* path);
};

class CArchivePsfStreamProvider : public CPsfStreamProvider
{
public:
									CArchivePsfStreamProvider(const char*);
	virtual							~CArchivePsfStreamProvider();

	Framework::CStream*				GetStreamForPath(const char*);

private:
	CPsfArchive*					m_archive;
};

CPsfStreamProvider*					CreatePsfStreamProvider(const char*);

#endif
