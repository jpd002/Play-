#ifndef _PSUSAVEIMPORTER_H_
#define _PSUSAVEIMPORTER_H_

#include "SaveImporterBase.h"

class CPsuSaveImporter : public CSaveImporterBase
{
public:
							CPsuSaveImporter();
	virtual					~CPsuSaveImporter();

	virtual void			Import(Framework::CStream&, const boost::filesystem::path&);
};

#endif
