#pragma once

#include "SaveImporterBase.h"
#include "McDumpReader.h"

class CMcDumpSaveImporter : public CSaveImporterBase
{
public:
	virtual ~CMcDumpSaveImporter() = default;

	void Import(Framework::CStream&, const fs::path&) override;

private:
	void ImportDirectory(CMcDumpReader&, const CMcDumpReader::Directory&, const fs::path&);
};
