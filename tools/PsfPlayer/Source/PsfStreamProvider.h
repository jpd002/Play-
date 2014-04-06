#pragma once

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

	Framework::CStream*				GetStreamForPath(const boost::filesystem::path&) override;
};

class CArchivePsfStreamProvider : public CPsfStreamProvider
{
public:
									CArchivePsfStreamProvider(const boost::filesystem::path&);
	virtual							~CArchivePsfStreamProvider();

	Framework::CStream*				GetStreamForPath(const boost::filesystem::path&) override;

private:
	std::unique_ptr<CPsfArchive>	m_archive;
};

std::unique_ptr<CPsfStreamProvider> CreatePsfStreamProvider(const boost::filesystem::path&);
