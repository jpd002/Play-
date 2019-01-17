#pragma once

#include <map>
#include <string>
#include <QPixmap>

class CoverUtils
{

private:
	static std::map<std::string, QPixmap> cache;

public:
	static void PopulateCache();
	static QPixmap find(std::string key);
	static void PopulatePlaceholderCover();
};
