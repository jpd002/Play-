#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include "filesystem_def.h"
#include "Stream.h"
#include "SaveImporterBase.h"

class CSaveImporter
{
public:
	typedef CSaveImporterBase::OverwritePromptHandlerType OverwritePromptHandlerType;

	static void ImportSave(Framework::CStream&, const fs::path&, const OverwritePromptHandlerType&);
};

#endif
