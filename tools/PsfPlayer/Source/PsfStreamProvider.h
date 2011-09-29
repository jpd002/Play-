#ifndef _PSFSTREAMPROVIDER_H_
#define _PSFSTREAMPROVIDER_H_

#include "PsfArchive.h"
#include "Stream.h"
#include <boost/filesystem/path.hpp>

class CPsfStreamProvider
{
public:
	virtual							~CPsfStreamProvider() {}
	virtual Framework::CStream*		GetStreamForPath(const boost::filesystem::path&) = 0;
};

class CPhysicalPsfStreamProvider : public CPsfStreamProvider
{
public:
	virtual							~CPhysicalPsfStreamProvider() {}

	Framework::CStream*				GetStreamForPath(const boost::filesystem::path&);
};

class CArchivePsfStreamProvider : public CPsfStreamProvider
{
public:
									CArchivePsfStreamProvider(const boost::filesystem::path&);
	virtual							~CArchivePsfStreamProvider();

	Framework::CStream*				GetStreamForPath(const boost::filesystem::path&);

private:
	CPsfArchive*					m_archive;
};

CPsfStreamProvider*					CreatePsfStreamProvider(const boost::filesystem::path&);

#endif
