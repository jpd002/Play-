#ifndef _XPSSAVEIMPORTER_H_
#define _XPSSAVEIMPORTER_H_

#include "SaveImporterBase.h"

class CXpsSaveImporter : public CSaveImporterBase
{
public:
	CXpsSaveImporter();
	virtual ~CXpsSaveImporter();

	void Import(Framework::CStream&, const fs::path&) override;

private:
	void ExtractFiles(Framework::CStream&, const fs::path&, uint32);
};

#endif
