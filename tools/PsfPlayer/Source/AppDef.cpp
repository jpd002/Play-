#include "AppConfig.h"
#include "PathUtils.h"

fs::path CAppConfig::GetBasePath() const
{
	static const char* BASE_DATA_PATH = "PsfPlayer Data Files";
	static const auto basePath =
	    []() {
		    auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
		    Framework::PathUtils::EnsurePathExists(result);
		    return result;
	    }();
	return basePath;
}
