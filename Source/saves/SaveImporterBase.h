#pragma once

#include "filesystem_def.h"
#include <functional>
#include "Stream.h"

class CSaveImporterBase
{
public:
	enum OVERWRITE_PROMPT_RETURN
	{
		OVERWRITE_YES,
		OVERWRITE_YESTOALL,
		OVERWRITE_NO
	};

	typedef std::function<OVERWRITE_PROMPT_RETURN(const fs::path&)> OverwritePromptHandlerType;

	CSaveImporterBase() = default;
	virtual ~CSaveImporterBase() = default;

	virtual void Import(Framework::CStream&, const fs::path&) = 0;

	void SetOverwritePromptHandler(const OverwritePromptHandlerType&);

protected:
	bool CanExtractFile(const fs::path&);

private:
	bool m_overwriteAll = false;
	OverwritePromptHandlerType m_overwritePromptHandler;
};
