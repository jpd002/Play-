#pragma once

#include "PsfArchive.h"
#include "StdStream.h"
#include "zip/ZipArchiveReader.h"

class CPsfZipArchive : public CPsfArchive
{
public:
										CPsfZipArchive();
	virtual								~CPsfZipArchive();
	
	virtual void						Open(const boost::filesystem::path&) override;
	virtual void						ReadFileContents(const char*, void*, unsigned int) override;

private:
	typedef std::unique_ptr<Framework::CZipArchiveReader> ZipArchiveReaderPtr;

	Framework::CStdStream				m_inputFile;
	ZipArchiveReaderPtr					m_archive;
};
