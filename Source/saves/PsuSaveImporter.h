#pragma once

#include "SaveImporterBase.h"

class CPsuSaveImporter : public CSaveImporterBase
{
public:
	virtual ~CPsuSaveImporter() = default;

	void Import(Framework::CStream&, const fs::path&) override;
};
