#ifndef _MAXSAVEIMPORTER_H_
#define _MAXSAVEIMPORTER_H_

#include "SaveImporterBase.h"

class CMaxSaveImporter : public CSaveImporterBase
{
public:
	virtual ~CMaxSaveImporter() = default;

	void Import(Framework::CStream&, const fs::path&) override;
};

#endif
