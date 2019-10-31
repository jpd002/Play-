#pragma once

#include "PsfArchive.h"
#include "PsfPathToken.h"
#include "Stream.h"
#include "filesystem_def.h"

class CPsfStreamProvider
{
public:
	virtual ~CPsfStreamProvider()
	{
	}
	virtual Framework::CStream* GetStreamForPath(const CPsfPathToken&) = 0;
	virtual CPsfPathToken GetSiblingPath(const CPsfPathToken&, const std::string&) = 0;
};

class CPhysicalPsfStreamProvider : public CPsfStreamProvider
{
public:
	virtual ~CPhysicalPsfStreamProvider()
	{
	}

	static CPsfPathToken GetPathTokenFromFilePath(const fs::path&);
	static fs::path GetFilePathFromPathToken(const CPsfPathToken&);

	Framework::CStream* GetStreamForPath(const CPsfPathToken&) override;
	CPsfPathToken GetSiblingPath(const CPsfPathToken&, const std::string&) override;
};

class CArchivePsfStreamProvider : public CPsfStreamProvider
{
public:
	CArchivePsfStreamProvider(const fs::path&);
	virtual ~CArchivePsfStreamProvider();

	static CPsfPathToken GetPathTokenFromFilePath(const std::string&);
	static std::string GetFilePathFromPathToken(const CPsfPathToken&);

	Framework::CStream* GetStreamForPath(const CPsfPathToken&) override;
	CPsfPathToken GetSiblingPath(const CPsfPathToken&, const std::string&) override;

private:
	std::unique_ptr<CPsfArchive> m_archive;
};

std::unique_ptr<CPsfStreamProvider> CreatePsfStreamProvider(const fs::path&);
