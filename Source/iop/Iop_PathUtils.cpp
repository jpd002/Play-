#include <cstring>
#include "Iop_PathUtils.h"

fs::path Iop::PathUtils::MakeHostPath(const fs::path& baseHostPath, const char* guestPath)
{
	if(strlen(guestPath) == 0)
	{
		//If we're not adding anything, just return whatever we had
		//We don't want to introduce a trailing slash since it will
		//break other stuff
		return baseHostPath;
	}
	auto result = baseHostPath;
	result.concat("/");
	result.concat(guestPath);
	return result;
}
