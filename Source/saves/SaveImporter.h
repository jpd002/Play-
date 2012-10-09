#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include <boost/filesystem.hpp>
#include "Stream.h"
#include "SaveImporterBase.h"

class CSaveImporter
{
public:
	typedef CSaveImporterBase::OverwritePromptHandlerType OverwritePromptHandlerType;

	static void			ImportSave(Framework::CStream&, const boost::filesystem::path&, const OverwritePromptHandlerType&);
};

#endif
