#include "SaveImporterBase.h"

CSaveImporterBase::CSaveImporterBase()
    : m_overwriteAll(false)
{
}

CSaveImporterBase::~CSaveImporterBase()
{
}

void CSaveImporterBase::SetOverwritePromptHandler(const OverwritePromptHandlerType& overwritePromptHandler)
{
	m_overwritePromptHandler = overwritePromptHandler;
}

bool CSaveImporterBase::CanExtractFile(const fs::path& filePath)
{
	if(!fs::exists(filePath)) return true;
	if(m_overwriteAll) return true;
	if(!m_overwritePromptHandler) return true;

	auto result = m_overwritePromptHandler(fs::absolute(filePath).string());

	switch(result)
	{
	case OVERWRITE_YESTOALL:
		m_overwriteAll = true;
	case OVERWRITE_YES:
		return true;
		break;
	case OVERWRITE_NO:
		return false;
		break;
	}

	return false;
}
