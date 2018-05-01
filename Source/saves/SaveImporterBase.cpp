#include "SaveImporterBase.h"

namespace filesystem = boost::filesystem;

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

bool CSaveImporterBase::CanExtractFile(const filesystem::path& filePath)
{
	if(!filesystem::exists(filePath)) return true;
	if(m_overwriteAll) return true;
	if(!m_overwritePromptHandler) return true;

	auto result = m_overwritePromptHandler(filesystem::absolute(filePath).string());

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
