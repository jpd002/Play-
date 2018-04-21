#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include "SaveImporterBase.h"
#include "Stream.h"
#include <boost/filesystem.hpp>

class CSaveImporter
{
public:
	typedef CSaveImporterBase::OverwritePromptHandlerType OverwritePromptHandlerType;

	static void ImportSave(Framework::CStream&, const boost::filesystem::path&, const OverwritePromptHandlerType&);
};

#endif
