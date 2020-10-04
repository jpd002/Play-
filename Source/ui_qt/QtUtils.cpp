#include "QtUtils.h"
#include "DiskUtils.h"
#include <QStringList>

QString QtUtils::GetDiscImageFormatsFilter()
{
	std::string fileFilter;
	const auto& extensions = DiskUtils::GetSupportedExtensions();
	for(auto extensionIterator = extensions.begin();
	    extensionIterator != extensions.end(); extensionIterator++)
	{
		if(extensionIterator != extensions.begin())
		{
			fileFilter += " ";
		}
		fileFilter += "*" + *extensionIterator;
	}
	return QString::asprintf("Disc images(%s)", fileFilter.c_str());
}
