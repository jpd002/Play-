#pragma once

#include "SaveImporterBase.h"

class CXpsSaveImporter : public CSaveImporterBase
{
public:
	virtual ~CXpsSaveImporter() = default;

	void Import(Framework::CStream&, const fs::path&) override;

private:
	void ExtractFiles(Framework::CStream&, const fs::path&, uint32);
};
