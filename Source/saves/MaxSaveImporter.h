#pragma once

#include "SaveImporterBase.h"

class CMaxSaveImporter : public CSaveImporterBase
{
public:
	virtual ~CMaxSaveImporter() = default;

	void Import(Framework::CStream&, const fs::path&) override;
};
