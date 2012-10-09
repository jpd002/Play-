#ifndef _MAXSAVEIMPORTER_H_
#define _MAXSAVEIMPORTER_H_

#include "SaveImporterBase.h"

class CMaxSaveImporter : public CSaveImporterBase
{
public:
							CMaxSaveImporter();
	virtual					~CMaxSaveImporter();

	virtual void			Import(Framework::CStream&, const boost::filesystem::path&);
};

#endif
