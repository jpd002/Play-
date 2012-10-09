#ifndef _XPSSAVEIMPORTER_H_
#define _XPSSAVEIMPORTER_H_

#include "SaveImporterBase.h"

class CXpsSaveImporter : public CSaveImporterBase
{
public:
							CXpsSaveImporter();
	virtual					~CXpsSaveImporter();

	virtual void			Import(Framework::CStream&, const boost::filesystem::path&);

private:
	void					ExtractFiles(Framework::CStream&, const boost::filesystem::path&, uint32);
};

#endif
